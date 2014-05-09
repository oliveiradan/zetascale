#include <fdf.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>


#define FDF_MAX_KEY_LEN 1979
#define NUM_OBJS 10000 //max mput in single operation
#define NUM_MPUTS 1000000 

static int cur_thd_id = 0;
static __thread int my_thdid = 0;
FDF_cguid_t cguid;
struct FDF_state *fdf_state;
int num_mputs =  NUM_MPUTS;
int num_objs = NUM_OBJS;
int obj_data_len = 0;
int use_mput = 1;
uint32_t flags_global = 0;
int num_thds = 1;

int cnt_id = 0;
void verify_stats(struct FDF_thread_state *thd_state, FDF_cguid_t cguid);

#if 0
static int 
my_cmp_cb(void *data, char *key1, uint32_t keylen1, char *key2, uint32_t keylen2)
{
    int x;
    int cmp_len;

    cmp_len = keylen1 < keylen2 ? keylen1: keylen2;

    x = memcmp(key1, key2, cmp_len);
    if (x != 0) {
        return x;
    }

    /* Equal so far, use len to decide */
    if (keylen1 < keylen2) {
        return -1;
    } else if (keylen1 > keylen2) {
        return 1;
    } else {
        return 0;
    }
}
#endif 

inline uint64_t
get_time_usecs(void)
{
        struct timeval tv = { 0, 0};
        gettimeofday(&tv, NULL);
        return ((tv.tv_sec * 1000 * 1000) + tv.tv_usec);
}


void
do_mput(struct FDF_thread_state *thd_state, FDF_cguid_t cguid,
	uint32_t flags, int key_seed)
{
	int i, j, k;
	FDF_status_t status;
	FDF_obj_t *objs = NULL; 
	uint64_t start_time;
	uint64_t num_fdf_writes = 0;
	uint64_t num_fdf_reads = 0;
	uint64_t num_fdf_mputs = 0;
	uint32_t objs_written = 0;
	char *data;
	uint64_t data_len;
	uint64_t key_num = 0;
	uint64_t mismatch = 0;

	objs = (FDF_obj_t *) malloc(sizeof(FDF_obj_t) * num_objs);
	if (objs == NULL) {
		printf("Cannot allocate memory.\n");
		exit(0);
	}
	memset(objs, 0, sizeof(FDF_obj_t) * num_objs);
	for (i = 0; i < num_objs; i++) {
		objs[i].key = malloc(FDF_MAX_KEY_LEN);
		if (objs[i].key == NULL) {
			printf("Cannot allocate memory.\n");
			exit(0);
		}
		objs[i].data = malloc(1024);
		if (objs[i].data == NULL) {
			printf("Cannot allocate memory.\n");
			exit(0);
		}
	}

	printf("Doing Mput in threads %d.\n", my_thdid);
	start_time = get_time_usecs();
	for (k = 1; k <= num_mputs; k++) {

		for (i = 0; i < num_objs; i++) {
			memset(objs[i].key, 0, FDF_MAX_KEY_LEN);
			sprintf(objs[i].key, "key_%d_%06"PRId64"", my_thdid, key_num);

			sprintf(objs[i].data, "key_%d_%06"PRId64"_%x", my_thdid, key_num, flags);
			objs[i].data_len = strlen(objs[i].data) + 1;
		
			if (flags == FDF_WRITE_MUST_EXIST) { 
				strncat(&objs[i].data[objs[i].data_len - 1], objs[i].data, objs[i].data_len - 1);
			}
			objs[i].data_len = strlen(objs[i].data) + 1;



			key_num += key_seed;
			objs[i].key_len = strlen(objs[i].key) + 1;
			objs[i].flags = 0;
			if (!use_mput) {
				status = FDFWriteObject(thd_state, cguid,
						        objs[i].key, objs[i].key_len,
							objs[i].data, objs[i].data_len, flags);
				if (status != FDF_SUCCESS) {
					printf("Write failed with %d errror.\n", status);
					assert(0);
				}
				num_fdf_writes++;

				status = FDFReadObject(thd_state, cguid,
						       objs[i].key, objs[i].key_len,
						       &data, &data_len);
				if (status != FDF_SUCCESS) {
					printf("Rread failed after write.\n");
					assert(0);
				}

				assert(objs[i].data_len == data_len);

				assert(memcmp(objs[i].data, data, data_len) == 0);
				FDFFreeBuffer(data);
			}
		}

		if (use_mput) {
			status = FDFMPut(thd_state, cguid, num_objs,
					 &objs[0], flags, &objs_written);
			if (status != FDF_SUCCESS) {
				printf("Failed to write objects using FDFMPut, status = %d.\n",
					status);
				assert(0);
				return ;
			}
			num_fdf_mputs++;
			for (i = 0; i < num_objs; i++) {
				status = FDFReadObject(thd_state, cguid,
						       objs[i].key, objs[i].key_len,
						       &data, &data_len);
				if (status != FDF_SUCCESS) {
					printf("Rread failed after write.\n");
					assert(0);
				}

				assert(objs[i].data_len == data_len);

				assert(memcmp(objs[i].data, data, data_len) == 0);
				FDFFreeBuffer(data);


			}
		}


	}



	num_fdf_reads = 0;
	
	j = 0;
	printf("Reading all objects put in thread = %d.\n", my_thdid);
	key_num = 0;
	for (k = 1; k <= num_mputs; k++) {

		for (i = 0; i < num_objs; i++) {
			memset(objs[i].key, 0, FDF_MAX_KEY_LEN);

			sprintf(objs[i].key, "key_%d_%06"PRId64"", my_thdid, key_num);
			sprintf(objs[i].data, "key_%d_%06"PRId64"_%x", my_thdid, key_num, flags);
			objs[i].data_len = strlen(objs[i].data) + 1;
		
			if (flags == FDF_WRITE_MUST_EXIST) { 
				strncat(&objs[i].data[objs[i].data_len - 1], objs[i].data, objs[i].data_len - 1);
			}
			objs[i].data_len = strlen(objs[i].data) + 1;

			key_num += key_seed;

			objs[i].key_len = strlen(objs[i].key) + 1;
			objs[i].data_len = strlen(objs[i].data) + 1;
			objs[i].flags = 0;

			status = FDFReadObject(thd_state, cguid,
					       objs[i].key, objs[i].key_len,
						&data, &data_len);
			if (status != FDF_SUCCESS) {
					printf("Read failed with %d errror. Key=%s.\n", status, objs[i].key);
					assert(0);
					exit(0);
			}

			if (data_len != objs[i].data_len) {
				printf("Object length of read object mismatch. Key=%s\n", objs[i].key);
				assert(0);
				mismatch++;
			}

			if (memcmp(data, objs[i].data, objs[i].data_len) != 0) {
				printf("Object data of read object mismatch.\n");	
				assert(0);
				mismatch++;
			}
			num_fdf_reads++;
			FDFFreeBuffer(data);
		}
	}

	printf("Verified the mput objects using reads, mismatch = %"PRId64".\n", mismatch);
#if 0
	key_num = 0;
	for (k = 1; k <= num_mputs; k++) {

		for (i = 0; i < num_objs; i++) {
			memset(objs[i].key, 0, FDF_MAX_KEY_LEN);

			sprintf(objs[i].key, "key_%d_%06"PRId64"", my_thdid, key_num);
			sprintf(objs[i].data, "key_%d_%06"PRId64"", my_thdid, key_num);

			key_num += key_seed;

			objs[i].key_len = strlen(objs[i].key) + 1;

			status = FDFDeleteObject(thd_state, cguid,
					       objs[i].key, objs[i].key_len);
			if (status != FDF_SUCCESS) {
				printf("Delete failed with %d errror.\n", status);
				assert(0);
				exit(0);
			}

		}

	}

	printf("Deleted objects successfully for thread = %d.\n", my_thdid);
#endif 
}

void *
write_stress(void *t)
{
	struct FDF_thread_state *thd_state;


	my_thdid = __sync_fetch_and_add(&cur_thd_id, 1);
	FDFInitPerThreadState(fdf_state, &thd_state);	
#if 1
	if (flags_global & FDF_WRITE_MUST_EXIST) {
		do_mput(thd_state, cguid, FDF_WRITE_MUST_NOT_EXIST, 1); //populate data if it is update case.
	}

	if (flags_global == 0) {
		do_mput(thd_state, cguid, FDF_WRITE_MUST_NOT_EXIST, 2); //sparsely populate for set case
	}
#endif 

	do_mput(thd_state, cguid, flags_global, 1);

	return NULL;
}


void launch_thds()
{
	int i;
	pthread_t thread_id[128];

	sleep(1);
	for(i = 0; i < num_thds; i++) {
		fprintf(stderr,"Creating thread %i\n",i );
		if( pthread_create(&thread_id[i], NULL, write_stress, NULL)!= 0 ) {
		    perror("pthread_create: ");
		    exit(1);
		}
	}

	for(i = 0; i < num_thds; i++) {
		if( pthread_join(thread_id[i], NULL) != 0 ) {
			perror("pthread_join: ");
			exit(1);
		} 
	}

}

void verify_stats(struct FDF_thread_state *thd_state, FDF_cguid_t cguid) {
    FDF_stats_t stats;
    fprintf( stderr, "Verifying stats \n");
    /* Check FDF container stats */
    FDFGetContainerStats(thd_state,cguid,&stats);  
    if( (use_mput) && (
        (stats.n_accesses[FDF_ACCESS_TYPES_MPUT] != ( num_mputs * num_thds )) ||
        (stats.btree_stats[FDF_BTREE_NUM_MPUT_OBJS] != (num_mputs * num_thds * num_objs) ) ||
        (stats.cntr_stats[FDF_CNTR_STATS_NUM_OBJS] != (num_mputs * num_thds * num_objs) )  ||
        (stats.cntr_stats[FDF_CNTR_STATS_USED_SPACE] == 0 ) ) ){
        fprintf( stderr, "Stats Verification failed:\n"
                         "num_puts     :Expected:%d Actual:%lu\n"
                         "num_mput_objs:Expected:%d Actual:%lu\n"
                         "num_objs     :Expected:%d Actual:%lu\n"
                         "used_space   :Expected:>0 Actual:%lu\n",
                         num_mputs * num_thds, stats.n_accesses[FDF_ACCESS_TYPES_MPUT],
                         num_mputs * num_thds * num_objs, stats.btree_stats[FDF_BTREE_NUM_MPUT_OBJS],
                         num_mputs * num_thds * num_objs, stats.cntr_stats[FDF_CNTR_STATS_NUM_OBJS],
                         stats.cntr_stats[FDF_CNTR_STATS_USED_SPACE]);
        //exit(-1);
    }  
}

FDF_cguid_t cguids[10000];
void
do_op(uint32_t flags_in) 
{
	FDF_container_props_t props;
	struct FDF_thread_state *thd_state;
	FDF_status_t status;
	int i = 0;
//	FDF_container_meta_t cmeta = {my_cmp_cb, NULL};

	char cnt_name[100] = {0};

	sprintf(cnt_name, "cntr_%d", cnt_id++);


	FDFInitPerThreadState(fdf_state, &thd_state);	

	FDFLoadCntrPropDefaults(&props);

	props.persistent = 1;
	props.evicting = 0;
	props.writethru = 1;
	props.durability_level= 0;
	props.fifo_mode = 0;
	props.size_kb = (1024 * 1024 * 10);;

	for (i = 0; i < 10000; i++) {
		sprintf(cnt_name, "cntr_%d", i);
		status = FDFOpenContainer(thd_state, cnt_name, &props, FDF_CTNR_CREATE, &cguids[i]);
	//	status = FDFOpenContainerSpecial(thd_state, cnt_name, &props,
	//					 FDF_CTNR_CREATE, &cmeta, &cguid);
		if (status != FDF_SUCCESS) {
			printf("Open Cont failed with error=%x.\n", status);
			exit(-1);
		}

	}

	flags_global = flags_in;
//	launch_thds(); //actual operations
//        verify_stats(thd_state,cguid);

//	FDFCheckBtree(thd_state, cguid);
	for (i = 0; i < 10000; i++) {
		FDFCloseContainer(thd_state, cguids[0]);
		FDFDeleteContainer(thd_state, cguids[0]);
	}

	FDFReleasePerThreadState(&thd_state);
}

int 
main(int argc, char *argv[])
{
	int n, m;

	if (argc < 5) {
		printf("Usage: ./run 0/1(use mput)  num_mputs num_objs_each_mput num_thds.\n");
		exit(0);
	}

	n = atoi(argv[4]);
	if (n > 0) {
		num_thds = n;
	}

	FDFInit(&fdf_state);
#if 1
	/* Test bulk insert */
	num_mputs = 5;
	for(num_objs = 500; num_objs < 501; num_objs++) { 
		for(obj_data_len = 100; obj_data_len < 101; obj_data_len+=3) { 
			printf("Running with mput (y/n) = %d, mputs = %d, num objs each mput = %d, num threads = %d. data_len=%d\n",
					use_mput, num_mputs, num_objs, num_thds, obj_data_len);

			printf(" ======================== Doing test for set case. ===================\n");
			do_op(0);// set
			printf(" ******************  Done test for set case.***********************\n");

			printf(" ======================== Doing test for create case. ===================\n");
			do_op(FDF_WRITE_MUST_NOT_EXIST); //create
			printf(" ******************  Done test for create  case.***********************\n");
#if 0
			printf(" ======================== Doing test for update case. ===================\n");
			do_op(FDF_WRITE_MUST_EXIST); //update
			printf(" ******************  Done test for update  case.***********************\n");
#endif
		}
	}
#endif 

	obj_data_len = 0;

	use_mput = atoi(argv[1]);
	m = atoi(argv[2]);
	if (m > 0) {
		num_mputs = m;	
	}
	n = atoi(argv[3]);
	if (n > 0) {
		num_objs = n;
	}

	printf("Running with mput (y/n) = %d, mputs = %d, num objs each mput = %d, num threads = %d.\n",
		use_mput, num_mputs, num_objs, num_thds);

	printf(" ======================== Doing test for set case. ===================\n");
	do_op(0);// set
	printf(" ******************  Done test for set case.***********************\n");

#if 1
	printf(" ======================== Doing test for create case. ===================\n");
	do_op(FDF_WRITE_MUST_NOT_EXIST); //create
	printf(" ******************  Done test for create  case.***********************\n");

	printf(" ======================== Doing test for update case. ===================\n");
	do_op(FDF_WRITE_MUST_EXIST); //update
	printf(" ******************  Done test for update  case.***********************\n");
#endif

	FDFShutdown(fdf_state);
	return 0;
}


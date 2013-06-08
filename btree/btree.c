/************************************************************************
 * 
 *  btree.c  Jan. 21, 2013   Brian O'Krafka
 * 
 * xxxzzz NOTES:
 *     - check all uses of "ret"
 *     - add "TLB" hash lookup for speed
 *     - add doxygen comments for all functions
 *     - make asserts a compile time option
 * 
 ************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "btree_hash.h"
#include "btree.h"
#include "btree_internal.h"


#define bt_err(msg, args...) \
    (bt->msg_cb)(0, 0, __FILE__, __LINE__, msg, ##args);
#define bt_warn(msg, args...) \
    (bt->msg_cb)(1, 0, __FILE__, __LINE__, msg, ##args);

static void default_msg_cb(int level, void *msg_data, char *filename, int lineno, char *msg, ...)
{
    char     stmp[512];
    va_list  args;
    char    *prefix;
    int      quit = 0;

    va_start(args, msg);

    vsprintf(stmp, msg, args);
    strcat(stmp, "\n");

    va_end(args);

    switch (level) {
        case 0:  prefix = "ERROR";                quit = 1; break;
        case 1:  prefix = "WARNING";              quit = 0; break;
        case 2:  prefix = "INFO";                 quit = 0; break;
        case 3:  prefix = "DEBUG";                quit = 0; break;
        default: prefix = "PROBLEM WITH MSG_CB!"; quit = 1; break;
	    break;
    } 

    (void) fprintf(stderr, "%s: %s", prefix, stmp);
    if (quit) {
        exit(1);
    }
}

static int default_cmp_cb(void *data, char *key1, uint32_t keylen1, char *key2, uint32_t keylen2)
{
    if (keylen1 < keylen2) {
        return(-1);
    } else if (keylen1 > keylen2) {
        return(1);
    } else if (keylen1 == keylen2) {
        return(memcmp(key1, key2, keylen1));
    }
    assert(0);
    return(0);
}

btree_t *btree_init(uint32_t n_partitions, uint32_t flags, uint32_t max_key_size, uint32_t min_keys_per_node, uint32_t nodesize, uint32_t n_l1cache_buckets, create_node_cb_t *create_node_cb, void *create_node_data, read_node_cb_t *read_node_cb, void *read_node_cb_data, write_node_cb_t *write_node_cb, void *write_node_cb_data, freebuf_cb_t *freebuf_cb, void *freebuf_cb_data, delete_node_cb_t *delete_node_cb, void *delete_node_data, log_cb_t *log_cb, void *log_cb_data, msg_cb_t *msg_cb, void *msg_cb_data, cmp_cb_t *cmp_cb, void * cmp_cb_data, txn_cmd_cb_t *txn_cmd_cb, void *txn_cmd_cb_data)
{
    int          i;
    btree_t     *bt;

    bt = (btree_t *) malloc(sizeof(btree_t));
    if (bt == NULL) {
        return(NULL);
    }

    /* the following is identical to what is done in btree_raw_init() */

    bt->flags                = flags;
    bt->max_key_size         = max_key_size;
    bt->min_keys_per_node    = min_keys_per_node;;
    bt->nodesize             = nodesize;
    bt->create_node_cb       = create_node_cb;
    bt->create_node_cb_data  = create_node_data;
    bt->read_node_cb         = read_node_cb;
    bt->read_node_cb_data    = read_node_cb_data;
    bt->write_node_cb        = write_node_cb;
    bt->write_node_cb_data   = write_node_cb_data;
    bt->freebuf_cb           = freebuf_cb;
    bt->freebuf_cb_data      = freebuf_cb_data;
    bt->delete_node_cb       = delete_node_cb;
    bt->delete_node_cb_data  = delete_node_data;
    bt->log_cb               = log_cb;
    bt->log_cb_data          = log_cb_data;
    bt->msg_cb               = msg_cb;
    bt->msg_cb_data          = msg_cb_data;
    if (msg_cb == NULL) {
	bt->msg_cb           = default_msg_cb;
	bt->msg_cb_data      = NULL;
    }
    bt->cmp_cb               = cmp_cb;
    bt->cmp_cb_data          = cmp_cb_data;
    if (cmp_cb == NULL) {
	bt->cmp_cb           = default_cmp_cb;
	bt->cmp_cb_data      = NULL;
    }

//    bt->txn_cmd_cb           = txn_cmd_cb;
//    bt->txn_cmd_cb_data      = txn_cmd_cb_data;

    if (min_keys_per_node < 4) {
	bt_err("min_keys_per_node must be >= 4");
        free(bt);
	return(NULL);
    }

    /* the following is specific to btree_t */

    bt->n_partitions = n_partitions;
    bt->partitions = (struct btree_raw **) malloc(n_partitions*sizeof(struct btree_raw *));
    if (bt->partitions == NULL) {
	bt_err("Could not allocate btree partitions");
        free(bt);
	return(NULL);
    }

    for (i=0; i<n_partitions; i++) {
	   bt->partitions[i] = btree_raw_init(flags, i, n_partitions, max_key_size, min_keys_per_node, nodesize, n_l1cache_buckets, create_node_cb, create_node_data, read_node_cb, read_node_cb_data, write_node_cb, write_node_cb_data, freebuf_cb, freebuf_cb_data, delete_node_cb, delete_node_data, log_cb, log_cb_data, msg_cb, msg_cb_data, cmp_cb, cmp_cb_data, txn_cmd_cb, txn_cmd_cb_data);
	   if (bt->partitions[i] == NULL) {
		   bt_warn("Failed to allocate a btree partition!");
		   /* cleanup */
		   // TODO xxxzzz
		   return(NULL);
	   }
	}

    bt->n_iterators      = 0;
    bt->n_free_iterators = 0;
    bt->free_iterators   = NULL;
    bt->used_iterators   = NULL;

    return(bt);
}

btree_status_t btree_destroy(struct btree *btree)
{
    // TODO xxxzzz
    return(0);
}

static uint64_t hash_key(char *key, uint32_t keylen)
{
    return(btree_hash((unsigned char *) key, keylen, 0)); // xxxzzz set the salt to something else?
}

btree_status_t btree_get(struct btree *btree, char *key, uint32_t keylen, char **data, uint64_t *datalen, btree_metadata_t *meta)
{
    int        n_partition = hash_key(key, keylen) % btree->n_partitions;

    return  btree_raw_get(btree->partitions[n_partition], key, keylen, data, datalen, meta);
}

btree_status_t btree_insert(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta)
{
    int        n_partition = hash_key(key, keylen) % btree->n_partitions;

    return btree_raw_insert(btree->partitions[n_partition], key, keylen, data, datalen, meta);
}

btree_status_t btree_update(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta)
{
    int n_partition = hash_key(key, keylen) % btree->n_partitions;

    return btree_raw_update(btree->partitions[n_partition], key, keylen, data, datalen, meta);
}

btree_status_t btree_set(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta)
{
    int n_partition = hash_key(key, keylen) % btree->n_partitions;

    return btree_raw_set(btree->partitions[n_partition], key, keylen, data, datalen, meta);
}

/*   delete a key
 *
 *   returns 0: success
 *   returns 1: key not found
 */
btree_status_t btree_delete(struct btree *btree, char *key, uint32_t keylen, btree_metadata_t *meta)
{
    int n_partition = hash_key(key, keylen) % btree->n_partitions;

    return btree_raw_delete(btree->partitions[n_partition], key, keylen, meta);
}

/* Like btree_get, but gets next n_in keys after a specified key.
 * Use key=NULL and keylen=0 for first call in enumeration.
 */
int btree_get_next_n(uint32_t n_in, uint32_t *n_out, struct btree *btree, char *key_in, uint32_t keylen_in, char **keys_out, uint32_t *keylens_out, char **data_out, uint64_t datalens_out, btree_metadata_t *meta)
{
    // TODO xxxzzz
    return(1);
}

int btree_free_buffer(struct btree *btree, char *key, uint32_t keylen, char *buf)
{
    uint64_t   h;
    int        ret;
    int        n_partition;

    h = hash_key(key, keylen);
    n_partition = h % btree->n_partitions;

    ret = btree_raw_free_buffer(btree->partitions[n_partition], buf);
    return(ret);
}

int btree_fast_build(struct btree *btree)
{
    // TODO xxxzzz
    return(1);
}

void btree_dump(FILE *f, struct btree *btree)
{
    // TODO xxxzzz
}

void btree_check(struct btree *btree)
{
    // TODO xxxzzz
}

void btree_test(struct btree *btree)
{
    // TODO xxxzzz
}

int btree_snapshot(struct btree *btree, uint64_t *seqno)
{
    // TODO xxxzzz
    return(1);
}

int btree_delete_snapshot(struct btree *btree, uint64_t seqno)
{
    // TODO xxxzzz
    return(1);
}

int btree_get_snapshots(struct btree *btree, uint32_t *n_snapshots, uint64_t *seqnos)
{
    // TODO xxxzzz
    return(1);
}

void btree_get_stats(struct btree *bt, btree_stats_t *stats_all)
{
    int            i, j;
    btree_stats_t  stats;

    memset(stats_all, 0, sizeof(btree_stats_t));
    for (i=0; i<bt->n_partitions; i++) {
	btree_raw_get_stats(bt->partitions[i], &stats);

	for (j=0; j<N_BTSTATS; j++) {
	    stats_all->stat[j] +=  stats.stat[j];
	}
    }

    for (j=0; j<N_BTSTATS; j++) {
	if (stats_all->stat[j] != 0) {
	    stats_all->stat[j] /= bt->n_partitions;
	}
    }
}

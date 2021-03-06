//----------------------------------------------------------------------------
// ZetaScale
// Copyright (c) 2016, SanDisk Corp. and/or all its affiliates.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published by the Free
// Software Foundation;
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License v2.1 for more details.
//
// A copy of the GNU Lesser General Public License v2.1 is provided with this package and
// can also be found at: http://opensource.org/licenses/LGPL-2.1
// You should have received a copy of the GNU Lesser General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
//----------------------------------------------------------------------------

/*
 * File:   fcnl_queue_order_fth_test1.c
 * Author: Norman Xu
 *
 * Created on June 30, 2008, 7:45AM
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 * $Id: fcnl_queue_order_fth_test2.c 308 2008-06-30 22:34:58Z normanxu $
 */

#ifdef MPI_BUILD
#include <mpi.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

#include "platform/logging.h"
#define PLAT_OPTS_NAME(name) name ## _mpilogme
#include "platform/opts.h"
#include "platform/shmem.h"
#include "platform/fcntl.h"
#include "fth/fth.h"
#include "fth/fthMbox.h"
#include "platform/errno.h"
#include "utils/properties.h"

#include "sdfmsg/sdf_msg_types.h"
#include "fcnl_test.h"
#include "log.h"
struct test_info * info;
int myid = -1, pnodeid = -1;

#define LOG_CAT PLAT_LOG_CAT_SDF_SDFMSG

pthread_t fthPthread;
int msgCount;


int main(int argc, char *argv[]) {
	info = (struct test_info *)malloc(sizeof(struct test_info));
	test_info_init(info);
	info->test_type = 0;
    info->msg_count=50;
    
    struct plat_opts_config_mpilogme config;
    SDF_boolean_t success = SDF_TRUE;
    uint32_t numprocs;
    int tmp, namelen, mpiv = 0, mpisubv = 0;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int msg_init_flags = SDF_MSG_MPI_INIT;

    config.inputarg = 0;
    config.msgtstnum = 50;

    /* We may not need to gather anything from here but what the heck */
    loadProperties("/opt/schooner/config/schooner-med.properties"); // TODO get filename from command line

    /* make sure this is first in order to get the the mpi init args */
    success = plat_opts_parse_mpilogme(&config, argc, argv) ? SDF_FALSE : SDF_TRUE;

    printf("input arg %d msgnum %d success %d\n", config.inputarg, config.msgtstnum, success);
    fflush(stdout);
    myid = sdf_msg_init_mpi(argc, argv, &numprocs, &success, msg_init_flags);
    info->myid = myid;
    if ((!success) || (myid < 0)) {
        printf("Node %d: MPI Init failure... exiting - errornum %d\n", myid,
                success);
        fflush(stdout);
        MPI_Finalize();
        return (EXIT_FAILURE);
    }
    tmp = init_msgtest_sm((uint32_t)myid);

    /* Enable this process to run threads across 7 cpus, MPI will default to running all threads
     * on only one core which is not what we really want as it forces the msg thread to time slice
     * with the fth threads that send and receive messsages
     * first arg is the number of the processor you want to start off on and arg #2 is the sequential
     * number of processors from there
     */
    lock_processor(0, 7);
    info->lock_cpu = 7;
    /* Startup SDF Messaging Engine FIXME - dual node mode still - pnodeid is passed and determined
     * from the number of processes mpirun sees.
     */
    sleep(1);

    /* dont enable message engine system management thread */
    msg_init_flags =  msg_init_flags | SDF_MSG_RTF_DISABLE_MNGMT;

    sdf_msg_init(myid, &pnodeid, msg_init_flags);
    MPI_Get_version(&mpiv, &mpisubv);
    MPI_Get_processor_name(processor_name, &namelen);

    printf("Node %d: MPI Version: %d.%d Name %s \n", myid, mpiv, mpisubv,
            processor_name);
    fflush(stdout);

    plat_log_msg(
            PLAT_LOG_ID_INITIAL,
            LOG_CAT,
            PLAT_LOG_LEVEL_TRACE,
            "\nNode %d: Completed Msg Init.. numprocs %d pnodeid %d Starting Test\n",
            myid, numprocs, pnodeid);

    for (msgCount = 0; msgCount < 2; msgCount++) {
        sleep(1);
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                "\nNode %d: Number of sleeps %d\n", myid, msgCount);
    }

 
    /* create the fth test threads */
    fthInit();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&fthPthread, &attr, &OrderTestFthPthreadRoutine, NULL);

    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
            "\nNode %d: Created pthread for FTH\n", myid);

    pthread_join(fthPthread, NULL);

    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
            "\nNode %d: SDF Messaging Test Complete\n", myid);

    /* Lets stop the messaging engine this will block until they complete */
    /* FIXME arg is the threadlvl */
    sdf_msg_stopmsg(myid, SYS_SHUTDOWN_SELF);

    plat_shmem_detach();

    info->success++;
    if (myid == 0) {
        sched_yield();
        printf("Node %d: Exiting message test after yielding... Calling MPI_Finalize\n", myid);
        fflush(stdout);
        sched_yield();
        MPI_Finalize();
        print_test_info(info);
        test_info_final(info);
    }
    else {
        printf("Node %d: Exiting message test... Calling MPI_Finalize\n", myid);
        fflush(stdout);
        sched_yield();
        MPI_Finalize();
    }
    printf("Successfully ends\n");
    return (EXIT_SUCCESS);
}

#include "platform/opts_c.h"

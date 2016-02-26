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
 * File:   fcnl_singleptl_chkmsg_lib1.c
 * Author: mac
 *
 * Created on Aug 1, 2008, 3:30 PM
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

#ifdef MPI_BUILD
#include <mpi.h>
#endif

#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

#include "platform/time.h"
#include "platform/logging.h"
#include "sdfmsg/sdf_fth_mbx.h"
#include "sdfmsg/sdf_msg_types.h"

#include "fcnl_test.h"
#include "Utilfuncs.h"
#include "log.h"

extern struct test_info * info;
extern uint32_t myid;

#define DBGP 0
#define TSZE 64
#define NANSEC 1.00e-9f
#define FASTPATH_TEST 0
#define ALLVALS 100

#define LOG_CAT PLAT_LOG_CAT_SDF_SDFMSG

/*
 * These fthreads will be used to simulate the SDF protocol sending and
 * receiving messages to the sdf messaging engine and across nodes
 *
 * @brief: 1(sender) -> 1(receiver) "singleprotocol"
 * Let node create a sender and a receiver, sender sends message to the peer node's receiver,
 * before sending message or after receive message all need check the content of message.
 */

static struct sdf_queue_pair *q_pair[SDF_PROTOCOL_COUNT];

static int mysync = 0;
static int cluster_node[MAX_NUM_SCH_NODES];
static struct startsync crp;
static startsync_t *ssync = &crp;

static int msgCount = 1, fthCount = 0;

/**
 * ptmsg is used to print the sdfmsg. It's very important that the message 
 * which the sender sends should be the receiver receives, so it's better check 
 * the field of the sdf message.
 */

#define ptmsg(msg); \
    printf("msg_version: %d, msg_clusterid: %d\n", msg->msg_version, msg->msg_clusterid); \
	printf("msg_src_service: %d, msg_dest_service: %d\n",msg->msg_src_service, msg->msg_dest_service); \
    printf("msg_src_vnode: %d, msg_dest_vnode: %d\n", msg->msg_src_vnode, msg->msg_dest_vnode); \
	printf("msg_type: %d, buff_seq: %d\n", msg->msg_type, msg->buff_seq); \
	printf("msg_flags: %d, msg_len: %d\n", msg->msg_flags, msg->msg_len); \
	printf("msg_conversation: %li, msg_seqnum: %li\n", msg->msg_conversation, msg->msg_seqnum); \
	printf("msg_in_order_ack: %li, msg_out_of_order_acks: %li\n", msg->msg_in_order_ack, msg->msg_out_of_order_acks); \
	printf("msg_out_of_order_nacks: %li, msg_timestamp: %li\n", msg->msg_out_of_order_nacks, msg->msg_timestamp); \
	printf("akrpmbx: %p, akrpmbx_from_req: %p\n", msg->akrpmbx, msg->akrpmbx_from_req); \
	printf("ndbin: %p, msg_sendstamp: %li\n", msg->ndbin, msg->msg_sendstamp); \
	printf("msg_payload: %p\n", msg->msg_payload); \
	printf("\n");

void
fthThreadSglPtlMsgChkSender(uint64_t arg) {
    int l = 0;
    vnode_t node;
    struct sdf_msg *send_msg = NULL;
    
    msg_type_t type = REQ_FLUSH;

    sdf_fth_mbx_t fthmbx;
    fthMbox_t ackmbox, respmbox;
    fthmbx.actlvl = SACK_BOTH_FTH;
    fthMboxInit(&ackmbox);
    fthMboxInit(&respmbox);
    fthmbx.abox = &ackmbox;
    fthmbx.rbox = &respmbox;
    printf("node %d, fth thread sender starting  %s: number of msgs to send = %d\n", myid, __func__, msgCount);
    fflush(stdout);

    int localpn, actmask;
    uint32_t numprocs;
    uint64_t ptl = arg;
    int localrank = sdf_msg_nodestatus(&numprocs, &localpn, cluster_node, &actmask);
    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d: numprocs %d active_procs mask 0x%x active_mask 0x%x\n", 
                  localrank, numprocs, localpn, actmask);
    if (numprocs == 1) {
        node = 0;
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
		     "\nNode %d: FASTPATH_TEST node %d myid %d\n", 
                     myid, node, myid);
    }
    else {
        node = local_get_pnode(localrank, localpn, numprocs);
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
	             "Node %d: %s my pnode is  %d\n", 
                     localrank, __func__, node);
	fflush(stdout);
	for (int i = 0; i < numprocs; i++) {
            plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                         "\nNode %d: %s cluster_node[%d] = %d\n", 
                         localrank, __func__, i, cluster_node[i]);
            fflush(stdout);
        }
    }
    q_pair[ptl] = local_create_myqpairs(ptl, myid, node);
    info->queue_pair_info->queue_add[0] = q_pair[ptl]->q_in;
    info->queue_pair_info->queue_add[1] = q_pair[ptl]->q_out;
    info->queue_pair_info->queue_pair_type = ptl;
    if (q_pair[ptl] == NULL) {
        fprintf(stderr, "%s: sdf_create_queue_pair %li failed\n", __func__, ptl);
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                     "\nNode %d: EXITING completed sending %d messages - mysync %d\n", 
                     myid, l, mysync);
        return;
    }
    

    plat_log_msg (PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d: created queue pair %p sn %d dn %d ss %li ds %li maxcnt %d\n",
                 myid, q_pair[ptl], myid, myid == 0 ? 1 : 0,
                 ptl, ptl, msgCount);
    

    FTH_SPIN_LOCK(&ssync->spin);
    mysync = 1;
    FTH_SPIN_UNLOCK(&ssync->spin);

    for (l = 0; l < msgCount; ++l) {
        int ret;

        send_msg = (struct sdf_msg *) sdf_msg_alloc(TSZE); 
        if (send_msg == NULL) {
             plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_FATAL,
                          "sdf_msg_alloc(TSZE) failed\n");
        }
        
        local_setmsg_payload(send_msg, TSZE, myid, l);
             
        type = REQ_FLUSH;
       
        ret = sdf_msg_send((struct sdf_msg *)send_msg, TSZE, node, ptl, myid, ptl, type, &fthmbx, NULL);
        ptmsg(send_msg);
	fthMboxWait(&ackmbox);
		
#if 0 
        if(myid == 0)
            printf("node %d, sender #%li sends %d times, sdf_msg_send return %d, message contents %c-%li\n", myid, ptl, l + 1, ret, l + 65, ptl);
        else
            printf("node %d, sender #%li sends %d times, sdf_msg_send return %d, message contents %c-%li\n", myid, ptl, l + 1, ret, l + 97, ptl);
#endif 

        sdf_msg_t * msg = (sdf_msg_t *)fthMboxWait(&respmbox);
        sdf_msg_free_buff(msg);
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                     "\nnode %d, sender #%li get the response, from node: %d\n", 
                     myid, ptl, msg->msg_src_vnode);
	if (ret != 0 )
        process_ret(ret, ptl, type, myid);

        fthYield(1);
    }

    printf("@@node %d, sender #%li sends %li protocol message finished, send %d times\n", myid, ptl, ptl, l);

    FTH_SPIN_LOCK(&ssync->spin); 
    fthCount ++; 
    FTH_SPIN_UNLOCK(&ssync->spin);

    while (fthCount != 2) {
        fthYield(100);
    }
    
    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d: EXITING completed sending %d messages - mysync %d\n", 
                 myid, l, mysync);
    printf("node %d, sender %li kill the scheduler.\n", myid, ptl);    
    fthKill(1);
}

/* 
 * This FTH thread will act as a protocol worker thread. 
 * It will wait for CONSISTENCY Messages 
 * on the queue, processes them and returns appropriate RESPONSE messages 
 */

void fthThreadSglPtlMsgChkRecver(uint64_t arg) {
    int ret, ct = 0;
    uint64_t aresp = 0, ptl;
    struct sdf_msg *recv_msg = NULL;
    vnode_t node;
    printf("node %d, fth thread receiver %li starting %s\n", myid, arg, __func__);
    fflush(stdout);

    int localpn, actmask;
    uint32_t numprocs;
    ptl = arg;
    int localrank = sdf_msg_nodestatus(&numprocs, &localpn, cluster_node, &actmask);
    if (localrank == localpn) {
        node = localpn;
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
		     "\nNode %d: FASTPATH_TEST node %d myid %d\n", myid, node, myid);
    }
    else {
        node = local_get_pnode(localrank, localpn, numprocs);
        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
	             "\nNode %d: my pnode is  %d\n", 
                     localrank, node);
	fflush(stdout);
    }
 		
    while (!mysync) {
         fthYield(1);
    }

    for (;;) {
        
        recv_msg = sdf_msg_receive(q_pair[ptl]->q_out, 0, B_TRUE);

        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                     "\nNode %d: Waiting for messages q_pair %p loop %d\n",
                     myid, q_pair[ptl], ct);

        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                     "\nNode %d: Got One *msg %p sn %d dn %d proto %d type %d"
                     " akrpmbx %p\n", myid, 
                     recv_msg, recv_msg->msg_src_vnode, recv_msg->msg_dest_vnode,
	             recv_msg->msg_dest_service, recv_msg->msg_type,
                     recv_msg->akrpmbx);

       ptmsg(recv_msg);
#if 0
    if(recv_msg) {
        uint32_t d = recv_msg->msg_dest_service;
        printf("node %d, receiver #%li recvs protocol#%d message from sender\n", myid, ptl, d);
        local_printmsg_payload(recv_msg, TSZE, myid);      
    }
    else {
        printf("!!node %d, receiver #%li recvs protocol#%d meessage from sender failed\n", myid, ptl, recv_msg->msg_dest_service);
    }    
#endif

        sdf_msg_t *send_msg = (struct sdf_msg *) sdf_msg_alloc(recv_msg->msg_len);
        memcpy(send_msg->msg_payload, recv_msg->msg_payload, recv_msg->msg_len - sizeof(sdf_msg_t));
	
    struct sdf_resp_mbx rhkey;
    struct sdf_resp_mbx *ptrkey = &rhkey;

    strncpy(rhkey.mkey, MSG_DFLT_KEY, (MSG_KEYSZE - 1));
    rhkey.mkey[MSG_KEYSZE - 1] = '\0';
    rhkey.akrpmbx_from_req = NULL;
    rhkey.rbox = NULL;
    
	sdf_msg_send((struct sdf_msg *)send_msg, TSZE, node, SDF_CONSISTENCY, myid, SDF_CONSISTENCY, RESP_ONE, NULL,
	                sdf_msg_get_response(recv_msg, ptrkey));

        ret = sdf_msg_free_buff(recv_msg);

        plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                     "\nNode %d: Send Buff Freed aresp %ld loop %d\n", 
                     myid, aresp, ct);
        ct++;
        
        if(ct == msgCount) break;

    }
    printf("@@node %d, receiver #%li, receive message finished, receive %d times\n", myid, ptl, ct);
     
    FTH_SPIN_LOCK(&ssync->spin);
    fthCount ++;
    FTH_SPIN_UNLOCK(&ssync->spin);
    
    printf("node: %d, protocol: %li, mysync: %d, fthCount: %d\n", myid, ptl, mysync, fthCount);
    fthYield(1);
    
    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d: WORKER FTH exiting - loop %d mysync %d\n", myid, ct, mysync);
    
}

void *
SinglePtlMsgContentChkPthreadRoutine(void *arg) {

    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d FTH threads firing up\n", myid);

    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d FTH scheduler has initialized -- Now Spawning fth threads\n", myid);

    XResume(XSpawn(&fthThreadSglPtlMsgChkRecver, 40960), SDF_CONSISTENCY);
    XResume(XSpawn(&fthThreadSglPtlMsgChkSender, 40960), SDF_CONSISTENCY);
    info->pthread_info = 1;
    info->fth_info = 2;
    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nNode %d Finished Create and Spawned -- Now starting sched\n", myid);

    fthSchedulerPthread(0);

    plat_log_msg(PLAT_LOG_ID_INITIAL, LOG_CAT, PLAT_LOG_LEVEL_TRACE,
                 "\nFTH scheduler halted\n");
    return (0);

}


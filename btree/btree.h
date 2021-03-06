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

/************************************************************************
 * 
 *  btree.h  Jan. 21, 2013   Brian O'Krafka
 * 
 *  btree code for a multi-threaded b-tree implementation that uses 
 *  multiple partitions implemented using the btree_raw single-threaded
 *  btree.
 * 
 * xxxzzz NOTES:
 *     - check all uses of "ret"
 *     - make sure that all btree updates are logged!
 *     - add doxygen comments for all functions
 *     - make asserts a compile time option
 *     - make sure that left/right node pointers are properly maintained
 *     - check insert_ptr arithmetic
 *     - optimize key search within a node?
 * 
 ************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include "btree_raw.h"


#ifndef __BTREE_H
#define __BTREE_H

typedef void (*zs_log_func) (const char *file, unsigned line, const char *function,
                              int logid, int category, int level, const char *format, ...);
extern zs_log_func zs_log_func_ptr;

#define LOG_CATAGORY_SDF_NAMING 12   
#define LOG_LEVEL_INFO 5   
#define PLAT_LOG_ID 160146

#define fprintf(fd,format,...) \
    if( zs_log_func_ptr != NULL ) { \
        zs_log_func_ptr(basename(__FILE__),__LINE__, __FUNCTION__, PLAT_LOG_ID,LOG_CATAGORY_SDF_NAMING,LOG_LEVEL_INFO, format,##__VA_ARGS__); \
    }\
    else { \
        fprintf(stderr,format,##__VA_ARGS__);\
    }\

struct btree;


extern struct btree *btree_init(uint32_t n_partitions, uint32_t flags, uint32_t max_key_size, uint32_t min_keys_per_node, uint32_t nodesize, 
				create_node_cb_t *create_node_cb, void *create_node_data, read_node_cb_t *read_node_cb, void *read_node_cb_data, 
 				write_node_cb_t *write_node_cb, void *write_node_cb_data, flush_node_cb_t *flush_node_cb, void *flush_node_cb_data, 
				freebuf_cb_t *freebuf_cb, void *freebuf_cb_data, delete_node_cb_t *delete_node_cb, void *delete_node_data, log_cb_t *log_cb, 
				void *log_cb_data, msg_cb_t *msg_cb, void *msg_cb_data, cmp_cb_t *cmp_cb, void * cmp_cb_data, bt_mput_cmp_cb_t mput_cmp_cb, 
				void *mput_cmp_cb_data, trx_cmd_cb_t *trx_cmd_cb, uint64_t cguid, zs_pstats_t *pstats, seqno_alloc_cb_t *ptr_seqno_alloc_cb);

extern btree_status_t btree_destroy(struct btree *btree, bool clean_l1cache);

extern btree_status_t btree_get(struct btree *btree, char *key, uint32_t keylen, char **data, uint64_t *datalen, btree_metadata_t *meta);

extern btree_status_t btree_insert(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta);

extern btree_status_t btree_update(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta);

extern btree_status_t btree_set(struct btree *btree, char *key, uint32_t keylen, char *data, uint64_t datalen, btree_metadata_t *meta);

extern btree_status_t btree_flush(struct btree *btree, char *key, uint32_t keylen);

/*   delete a key
 *
 */
extern btree_status_t btree_delete(struct btree *btree, char *key, uint32_t keylen, btree_metadata_t *meta);

/* Like btree_get, but gets next n_in keys after a specified key.
 * Use key=NULL and keylen=0 for first call in enumeration.
 */
extern int btree_get_next_n(uint32_t n_in, uint32_t *n_out, struct btree *btree, char *key_in, uint32_t keylen_in, char **keys_out, uint32_t *keylens_out, char **data_out, uint64_t datalens_out, btree_metadata_t *meta);

extern int btree_fast_build(struct btree *btree);

extern void btree_dump(FILE *f, struct btree *btree);

//extern void btree_check(struct btree *btree);

extern void btree_test(struct btree *btree);

extern int btree_snapshot(struct btree *btree, uint64_t *seqno);
extern int btree_delete_snapshot(struct btree *btree, uint64_t seqno);
extern int btree_get_snapshots(struct btree *btree, uint32_t *n_snapshots, uint64_t *seqnos);
extern int btree_free_buffer(struct btree *btree, char *key, uint32_t keylen, char *buf);

extern void btree_get_stats(struct btree *btree, btree_stats_t *stats);

btree_status_t
btree_mput(struct btree *btree, btree_mput_obj_t *objs,
	   uint32_t num_objs, uint32_t flags,
	   btree_metadata_t *meta, uint32_t *objs_written);

btree_status_t
btree_range_update(struct btree *btree, 
	       	   btree_metadata_t *meta,
		   char *range_key,
		   uint32_t range_key_len,
		   btree_rupdate_cb_t callback_func,
		   void * callback_args,	
		   btree_range_cmp_cb_t range_cmp_cb,
		   void *range_cmp_cb_args,
		   uint32_t *objs_updated,
		   btree_rupdate_marker_t **marker);

btree_rupdate_marker_t *
btree_alloc_rupdate_marker(struct btree * bt);
void
btree_free_rupdate_marker(struct btree *btree, btree_rupdate_marker_t *marker);

btree_status_t btree_ioctl(struct btree *btree, uint32_t ioctl_type, void *data);
bool
btree_check(struct btree *btree, uint64_t *num_objs);

btree_status_t btree_move_lasterror(struct btree *btree, void **pp_err_context, uint32_t *p_err_size);
btree_status_t btree_rescue(struct btree *btree, void *p_err_context);

#endif // __BTREE_H

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
 * File:   fifo.h
 * Author: Brian O'Krafka
 *
 * Created on January 8, 2009
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 * $Id: fifo.h 9639 2009-10-09 19:38:11Z jmoilanen $
 */

#ifndef _FIFO_H
#define _FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssd_fifo_ops {

    struct flashDev   * (*flashOpen)( char * devName, flash_settings_t *flash_settings, int flags );

    struct shard      * (*shardCreate)( struct flashDev * dev, 
                                        uint64_t shardID,
                                        int flags, 
                                        uint64_t quota, 
                                        uint64_t maxObjs );

    struct shard      * (*shardOpen)( struct flashDev * dev, 
                                      uint64_t shardID );

    int                 (*flashGet)( struct ssdaio_ctxt * pctxt, 
                                     struct shard * shard, 
                                     struct objMetaData * metaData,
                                     char * key, 
                                     char ** dataPtr, 
                                     int flags );
    
    int                 (*flashPut)( struct ssdaio_ctxt * pctxt, 
                                     struct shard * shard, 
                                     struct objMetaData * metaData, 
                                     char * key, 
                                     char * data, 
                                     int flags);

    int                 (*flashPutV)( struct ssdaio_ctxt * pctxt, 
                                     struct shard * shard, 
                                     struct objMetaData * metaData, 
                                     char ** key, 
                                     char ** data, 
                                     int count,
                                     int flags);


    int                 (*flashFreeBuf)( void * buf );

    uint64_t            (*flashStats)( struct shard * shard, int key );

    void                (*shardSync)( struct shard * shard );

    void                 (*shardClose)( struct shard * shard );

    int                 (*shardDelete)( struct shard * shard );

    int                 (*shardStart)( struct shard * shard );

    int                 (*shardStop)( struct shard * shard );

    uint64_t            (*flashGetHighSequence)( struct shard * shard );

    void                (*flashSetSyncedSequence)( struct shard * shard,
                                                   uint64_t seqno );

    int 		(*flashGetIterationCursors)(struct shard *shard, uint64_t seqno_start,
						    uint64_t seqno_len, uint64_t seqno_max,
                                                    const struct flashGetIterationResumeOutput * resume_cursor_in,
						    struct flashGetIterationOutput ** cursors_out);
    int 		(*flashGetByCursor)(struct shard *shard, int cursor_len, const void *cursor,
					    struct objMetaData *metaData, char **key, void **data, int flags, time_t flush_time);
    uint64_t            (*flashGetRetainedTombstoneGuarantee)(struct shard *shard);
    void                (*flashRegisterSetRetainedTombstoneGuaranteeCallback)(void (*callback)(uint64_t shardID, uint64_t seqno));

} ssd_fifo_ops_t;


extern ssd_fifo_ops_t   Ssd_fifo_ops;


extern struct flashDev *fifo_flashOpen(char *name, flash_settings_t *flash_settings, int flags);
extern struct shard *fifo_shardCreate(struct flashDev *dev, uint64_t shardID, 
	  int flags, uint64_t quota, unsigned maxObjs);
extern struct shard *fifo_shardOpen(struct flashDev *dev, uint64_t shardID );
extern int fifo_flashGet(struct ssdaio_ctxt *pctxt, struct shard *shard, struct objMetaData *metaData, 
	  char *key, char **dataPtr, int flags);
extern int fifo_flashPut(struct ssdaio_ctxt *pctxt, struct shard *shard, struct objMetaData *metaData, 
	  char *key, char *data, int flags);
extern int fifo_flashPutV(struct ssdaio_ctxt *pctxt, struct shard *shard, struct objMetaData *metaData, 
	  char **key, char **data, int count, int flags);
extern int fifo_flashFreeBuf(void *p);
extern uint64_t fifo_flashStats( struct shard * shard, int key );
extern void fifo_shardSync(shard_t *shard);
extern void  fifo_shardClose(shard_t *shard);
extern int  fifo_shardDelete(shard_t *shard);
extern int  fifo_shardStart(shard_t *shard);
extern int  fifo_shardStop(shard_t *shard);
extern uint64_t fifo_flashGetHighSequence(shard_t *shard);
extern void fifo_flashSetSyncedSequence(shard_t *shard, uint64_t seqno);
extern int fifo_flashGetIterationCursors(struct shard *shard, uint64_t seqno_start,
					 uint64_t seqno_len, uint64_t seqno_max,
					 const resume_cursor_t * resume_cursor_in,
					 struct flashGetIterationOutput **cursors_out);
extern int fifo_flashGetByCursor(struct shard *shard, int cursor_len, const void *cursor,
				 struct objMetaData *metaData, char **key, void **data, int flags, time_t flush_time);
extern uint64_t fifo_flashGetRetainedTombstoneGuarantee(struct shard *shard);
    extern void fifo_flashRegisterSetRetainedTombstoneGuaranteeCallback(void (*callback)(uint64_t shardID, uint64_t seqno));


#ifdef	__cplusplus
}
#endif

#endif /* _FIFO_H */

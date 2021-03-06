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
 * More ZS code.
 *
 * This defines:
 *   ZSEnumerateContainerObjects()
 *   ZSNextEnumeratedObject()
 *   ZSFinishEnumeration()
 *
 * Author: Johann George
 *
 * Copyright (c) 2013 SanDisk Corporation.  All rights reserved.
 */
#include "zs.h"
#include "lc.h"
#include "shared/private.h"
#include "shared/name_service.h"
#include "protocol/action/recovery.h"
#include "protocol/protocol_common.h"
#include "protocol/action/action_thread.h"


/*
 * Functions defined in fdf.c
 */
extern ZS_status_t zs_get_ctnr_status(ZS_cguid_t cguid, int delete_ok);
extern int zs_get_ctnr_from_cguid(ZS_cguid_t cguid);
extern inline void zs_incr_io_count( ZS_cguid_t cguid );
extern inline void zs_decr_io_count( ZS_cguid_t cguid );

/*
 * Convert a cguid to a shard.
 */
ZS_status_t
cguid_to_shard(SDF_action_init_t *pai, ZS_cguid_t cguid, shard_t **shard_ptr, int delete_ok)
{
    ZS_status_t s;
    SDF_container_meta_t meta;

    if ( (s = zs_get_ctnr_status(cguid, delete_ok)) != ZS_CONTAINER_OPEN )
        return s;
    
    s = name_service_get_meta(pai, cguid, &meta);
    if (s != ZS_SUCCESS)
        return s;

    flashDev_t *dev = 
    #ifdef MULTIPLE_FLASH_DEV_ENABLED
	get_flashdev_from_shardid(sdf_shared_state.config.flash_dev,
                                  meta.shard,
                                  sdf_shared_state.config.flash_dev_count);
    #else
	sdf_shared_state.config.flash_dev;
    #endif

    shard_t *shard = shardFind(dev, meta.shard);
    if (!shard)
        return ZS_SHARD_NOT_FOUND;

    *shard_ptr = shard;
    return ZS_SUCCESS;
}


/*
 * Prepare for the enumeration of all objects in a container.
 */
ZS_status_t
ZSEnumerateContainerObjects(struct ZS_thread_state *ts,
	                     ZS_cguid_t   cguid,
	                     struct ZS_iterator  **iter)
{
    ZS_status_t s;
    struct shard *shard = NULL;

    if ( !ts || !cguid || !iter ) {
        if ( !ts ) {
            plat_log_msg(80049,PLAT_LOG_CAT_SDF_NAMING,
                         PLAT_LOG_LEVEL_DEBUG,"ZS Thread state is NULL");
        }
        if ( !cguid ) {
            plat_log_msg(80050,PLAT_LOG_CAT_SDF_NAMING,
                  PLAT_LOG_LEVEL_DEBUG,"Invalid container cguid:%lu",cguid);
        }
        if ( !iter ) {
            plat_log_msg(80051,PLAT_LOG_CAT_SDF_NAMING,
              PLAT_LOG_LEVEL_DEBUG, "The argument ZS_iterator is NULL");
        }
        return ZS_INVALID_PARAMETER;
    }
    SDF_action_init_t *pai = (SDF_action_init_t *) ts;

    s = cguid_to_shard(pai, cguid, &shard, 0);
    if (s != ZS_SUCCESS)
        return s;

    return (enumerate_init( pai, shard, cguid, iter));
}


/*
 * Prepare for the enumeration of all objects in a PG.
 */
ZS_status_t
ZSEnumeratePGObjects( struct ZS_thread_state *t, ZS_cguid_t c, struct ZS_iterator **iter, char *k, uint32_t kl)
{
	ZS_status_t     s;

	if (!t || !c || !iter) {
		if (!t)
			plat_log_msg(80049, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "ZS Thread state is NULL");
		if (!c)
			plat_log_msg(80050, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "Invalid container cguid:%lu", c);
		if (!iter)
			plat_log_msg(80051, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "The argument ZS_iterator is NULL");
		return (ZS_INVALID_PARAMETER);
	}
	cntr_map_t *cmap = get_cntr_map( c);
	if (cmap == 0)
		return (ZS_CONTAINER_UNKNOWN);
	if (cmap->lc) {
		s = lc_enum_start( t, c, iter, k, kl);
		rel_cntr_map( cmap);
		return (s);
	}
	rel_cntr_map( cmap);
	return (ZS_FAILURE_INVALID_CONTAINER_TYPE);
}

ZS_status_t
ZSEnumerateAllPGObjects( struct ZS_thread_state *t, ZS_cguid_t c, struct ZS_iterator **iter)
{
	ZS_status_t     s;

	if (!t || !c || !iter) {
		if (!t)
			plat_log_msg(80049, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "ZS Thread state is NULL");
		if (!c)
			plat_log_msg(80050, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "Invalid container cguid:%lu", c);
		if (!iter)
			plat_log_msg(80051, PLAT_LOG_CAT_SDF_NAMING, PLAT_LOG_LEVEL_DEBUG, "The argument ZS_iterator is NULL");
		return (ZS_INVALID_PARAMETER);
	}
	cntr_map_t *cmap = get_cntr_map( c);
	if (cmap == 0)
		return (ZS_CONTAINER_UNKNOWN);
	if (cmap->lc) {
		s = lc_enum_start( t, c, iter, NULL, 0);
		rel_cntr_map( cmap);
		return (s);
	}
	rel_cntr_map( cmap);
	return (ZS_FAILURE_INVALID_CONTAINER_TYPE);
}


/*
 * Return the next enumerated object in a container.
 */
ZS_status_t
ZSNextEnumeratedObject(struct ZS_thread_state *ts,
	                struct ZS_iterator *iter,
	                char      **key,
	                uint32_t   *keylen,
	                char      **data,
	                uint64_t   *datalen)
{
    ZS_status_t s;
    uint64_t keylen64;

    if ( !ts || !iter ) {
        if ( !ts ) {
            plat_log_msg(80049,PLAT_LOG_CAT_SDF_NAMING,
                         PLAT_LOG_LEVEL_DEBUG,"ZS Thread state is NULL");
        }
        if ( !iter ) {
            plat_log_msg(80051,PLAT_LOG_CAT_SDF_NAMING,
               PLAT_LOG_LEVEL_DEBUG, "The argument ZS_iterator is NULL");
        }
        return ZS_INVALID_PARAMETER;
    }

    if ( (s = zs_get_ctnr_status(get_e_cguid(iter), 0)) != ZS_CONTAINER_OPEN ) {
        return s;
    }
    cntr_map_t *cmap = get_cntr_map( get_e_cguid( iter));
    if (cmap == 0)
	return (ZS_CONTAINER_UNKNOWN);
    if (cmap->lc) {
	s = lc_enum_next( ts, iter, key, keylen, data, datalen);
	rel_cntr_map( cmap);
	return (s);
    }
    rel_cntr_map( cmap);
#ifdef CMAP
   	zs_incr_io_count( get_e_cguid(iter) );
#endif /* CMAP */
    SDF_action_init_t *pai = (SDF_action_init_t *) ts;
    
    s = enumerate_next(pai, iter, key, &keylen64, data, datalen);

#ifdef CMAP
   	zs_decr_io_count(get_e_cguid(iter));
#endif /* CMAP */

    if (s != ZS_SUCCESS)
        return s;

    *keylen = keylen64;
    return ZS_SUCCESS;
}


/*
 * Finish enumeration of a container.
 */
ZS_status_t
ZSFinishEnumeration(struct ZS_thread_state *ts, struct ZS_iterator *iter)
{
    ZS_status_t s;

    if ( !ts || !iter ) {
        if ( !ts ) {
            plat_log_msg(80049,PLAT_LOG_CAT_SDF_NAMING,
                         PLAT_LOG_LEVEL_DEBUG,"ZS Thread state is NULL");
        }
        if ( !iter ) {
            plat_log_msg(80051,PLAT_LOG_CAT_SDF_NAMING,
               PLAT_LOG_LEVEL_DEBUG, "The argument ZS_iterator is NULL");
        }
        return ZS_INVALID_PARAMETER;
    }
    cntr_map_t *cmap = get_cntr_map( get_e_cguid( iter));
    if (cmap == 0)
	return (ZS_CONTAINER_UNKNOWN);
    if (cmap->lc) {
	s = lc_enum_finish( iter);
	rel_cntr_map( cmap);
	return (s);
    }
    rel_cntr_map( cmap);
    return (enumerate_done( (SDF_action_init_t *)ts, iter));
}

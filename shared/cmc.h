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
 * File:   cmc.h
 * Author: DO
 *
 * Created on January 15, 2008, 2:46 PM
 *
 * Copyright Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 * $Id: cmc.h 9071 2009-09-04 17:38:33Z briano $
 *
 */

#ifndef _CMC_H
#define _CMC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "container.h"
#include "container_props.h"
#include "container_meta.h"
#include "utils/hashmap.h"

__BEGIN_DECLS

// These are used to change the type of metadata object access
#define CMC_HASHMAP 0
#define CMC_PIN 1
#define CMC_BUFFERED 2


typedef struct {
    char cmc_path[MAX_CNAME_SIZE];
    SDF_container_meta_t meta;
    SDFContainer c;
    uint32_t node;
    SDF_boolean_t initialized;
} SDF_cmc_t;

/**
 * @brief Create a container metadata container (CMC).
 *
 * @param cmc_path <IN> The CMC name.
 * @return Pointer to the CMC object.
 */
SDF_cmc_t *
cmc_create(SDF_internal_ctxt_t *pai, const char *cmc_path);

/**
 * @brief Destroy a container metadata container (CMC).
 *
 * @param cmc <IN> Pointer to the CMC object.
 * @return SDF status.
 */
SDF_status_t
cmc_destroy(SDF_internal_ctxt_t *pai, SDF_cmc_t *cmc);

/**
 * @brief Create the container metadata object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cname <IN> Container name.
 * @param cguid <IN> Container guid.
 * @param meta <IN> Pointer to the container metadata object.
 * return SDF status.
 */
SDF_status_t
cmc_create_meta(SDF_internal_ctxt_t *pai, SDFContainer c, const char *cname, 
		SDF_cguid_t cguid, SDF_container_meta_t *meta);

/**
 * @brief Get the container metadata object for the container cguid.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @param meta <OUT> Pointer to a user-supplied meta data buffer.
 * return SDF status.
 */
SDF_status_t
cmc_get_meta(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid, 
	     SDF_container_meta_t *meta);

/**
 * @brief Get the container metadata object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cname <IN> Container name.
 * @param meta <OUT> Pointer to a user-supplied meta data buffer.
 * return SDF status.
 */
SDF_status_t
cmc_get_meta_from_cname(SDF_internal_ctxt_t *pai, SDFContainer c, const char *cname,
			SDF_container_meta_t *meta);

/**
 * @brief Remove the container object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return SDF status.
 */
SDF_status_t
cmc_remove_meta(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid);

/**
 * @brief Lock a container metadata object.
 *
 * This is an fth lock.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return SDF status.
 */
SDF_status_t
cmc_lock_meta(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid);

/**
 * @brief Unlock a container metadata object.
 *
 * This is an fth unlock.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return SDF status.
 */
SDF_status_t
cmc_unlock_meta(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid);

/**
 * @brief Determine the existance of a metadata object.
 *
 * @param c <IN> CMC open container pointer.
 * @param cname <IN> Container name.
 * @return SDF status (SDF_SUCCESS if exists, SDF_FAILURE if it does not).
 */
SDF_status_t
cmc_meta_exists(SDF_internal_ctxt_t *pai, SDFContainer c, const char *cname);

/**
 * @brief Determine the existance of a cguid.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return SDF status (SDF_SUCCESS if exists, SDF_FAILURE if it does not).
 */
SDF_status_t
cmc_cguid_exists(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid);

/**
 * @brief Put the container metadata object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @param meta <IN> Pointer to the container metadata object.
 * @return SDF status.
 */
SDF_status_t
cmc_put_meta(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid, 
	     SDF_container_meta_t *meta);

/**
 * @brief Get the container properties object for the named container.
 *

 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @param props <OUT> Pointer to a copy of the container properties object. Caller must free.
 * @return SDF status.
 */
SDF_status_t
cmc_get_props(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid, 
	      SDF_container_props_t *props);

/**
 * @brief Put the container properties object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @param props <OUT> Pointer to the container properties object.
 * @return SDF status.
 */
SDF_status_t
cmc_put_props(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid, 
	      SDF_container_props_t props);

/**
 * @brief Get the container shard mapping table object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return Shard, #SDF_SHARDID_INVALID on failure
 */
SDF_shardid_t
cmc_get_shard(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid);

/**
 * @brief Put the container shard mapping table object for the named container.
 *
 * @param c <IN> CMC open container pointer.
 * @param cguid <IN> Container guid.
 * @return Pointer to the container shard mapping table object.
 * @return SDF status.
 */
SDF_status_t
cmc_put_shard(SDF_internal_ctxt_t *pai, SDFContainer c, SDF_cguid_t cguid,
              SDF_shardid_t shard);

/**
 * @brief Delete all shards from a container. Assumes the container metadata is locked.
 *
 * @param c <IN> CMC open container pointer.
 * @param cname <IN> Container name.
 * @return SDF status.
 */
SDF_status_t
cmc_delete_shards(SDF_internal_ctxt_t *pai, SDFContainer c, const char *cname);

/**
 * @brief Get the cguid from the container name.
 *
 * @param c <IN> CMC open container pointer.
 * @param cname <IN> Container name.
 * @return cguid.
 */
SDF_cguid_t
cmc_get_cguid_from_cname(SDF_internal_ctxt_t *pai, SDFContainer c, const char *cname);

/**
 * @brief Create an object container
 *
 * @param path <IN> Container name.
 * @param p <IN> Container properties.
 * @return SDF status.
 */
SDF_status_t
cmc_create_object_container(SDF_internal_ctxt_t *pai, const char *path, SDF_container_props_t *p);

/**
 * @brief Delete an object container
 *
 * @param path <IN> Container name.
 * @return SDF status.
 */
SDF_status_t
cmc_delete_object_container(SDF_internal_ctxt_t *pai, const char *path);

/**
 * @brief Open an object container
 *
 * @param path <IN> Container name.
 * @param mode <IN> mode to open the container in (e.g. read/write/append/read-write).
 * @param container <OUT> pointer to the client-accessible container handle.
 * @return SDF_CONTAINER (pointer to server-accessible container handle).
 */
#ifdef SDFAPI
SDF_status_t 
cmc_open_object_container(
	SDF_internal_ctxt_t *pai,
	SDF_cguid_t	     cguid,
	SDF_container_mode_t mode
	);
#else
SDF_CONTAINER
cmc_open_object_container(SDF_internal_ctxt_t *pai, const char *path, 
			  SDF_container_mode_t mode, SDFContainer *container);
#endif /* SDFAPI */

#ifdef SDFAPI
SDF_status_t
cmc_close_object_container(SDF_internal_ctxt_t *pai, SDF_cguid_t cguid);
#else
SDF_status_t
cmc_close_object_container(SDF_internal_ctxt_t *pai, SDFContainer container);
#endif


/**
 * @brief Flush and invalidate a container's objects.
 *
 * @param path <IN> Container name.
 * @return SDF_status.
 */
SDF_status_t
cmc_flush_inval_object_container(SDF_internal_ctxt_t *pai, const char *path);

/**
 * @brief Invalidate a container's objects.
 *
 * @param path <IN> Container name.
 * @return SDF_status.
 */
SDF_status_t
cmc_inval_object_container(SDF_internal_ctxt_t *pai, const char *path);

/**
 * @brief Create/put a buffered object.
 *
 * @param container <IN> Container handle.
 * @param objkey <IN> Object key.
 * @param size <IN> Object size.
 * @param pbuf <IN> Pointer to object buffer.
 * @return SDF status.
 */
SDF_status_t
cmc_create_put_buffered_object(SDF_internal_ctxt_t *pai, SDFContainer container,
			       const char *objkey, SDF_size_t size, void *pbuf);

/**
 * @brief Put a buffered object.
 *
 * @param container <IN> Container handle.
 * @param objkey <IN> Object key.
 * @param size <IN> Object size.
 * @param pbuf <IN> Pointer to object buffer.
 * @return SDF status.
 */
SDF_status_t
cmc_put_buffered_object(SDF_internal_ctxt_t *pai, SDFContainer container,
			const char *objkey, SDF_size_t size, void *pbuf);

/**
 * @brief Get a buffered object for read access.
 *
 * @param pai <IN> Action agent context.
 * @param container <IN> Container handle.
 * @param objkey <IN> Object key.
 * @param pbuf <IN> Pointer to object buffer.
 * @param size <IN> Object size.
 * @param destLen <IN> Pointer to actual object size.
 * @return SDF status.
 */
SDF_status_t
cmc_get_for_read_buffered_object(SDF_internal_ctxt_t *pai, SDFContainer container,
				 const char *objkey, void *pbuf, SDF_size_t size,
				 SDF_size_t *destLen);

/**
 * @brief Get a buffered object for write access.
 *
 * @param container <IN> Container handle.
 * @param objkey <IN> Object key.
 * @param pbuf <IN> Pointer to object buffer.
 * @param size <IN> Object size.
 * @param destLen <IN> Pointer to actual object size.
 * @return SDF status.
 */
SDF_status_t
cmc_get_for_write_buffered_object(SDF_internal_ctxt_t *pai, SDFContainer container,
				  const char *objkey, void *pbuf, SDF_size_t size,
				  SDF_size_t *destLen);

/**
 * @brief Remove an object.
 *
 * @param container <IN> Container handle.
 * @param objkey <IN> Object key.
 * @return SDF status.
 */
SDF_status_t
cmc_remove_object(SDF_internal_ctxt_t *pai, SDFContainer container, const char *objkey);

/*
** Temporary
*/
SDF_CONTAINER
cmc_open_object_container_path(SDF_internal_ctxt_t *pai, const char *path, 
			      SDF_container_mode_t mode, SDFContainer *container);
SDF_status_t
cmc_close_object_container_path(SDF_internal_ctxt_t *pai, SDFContainer container);

__END_DECLS

#ifdef __cplusplus
}
#endif

#endif /* _CMC_H */

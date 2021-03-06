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
 * File:   XLock.c
 * Author: Jim
 *
 * Created on February 29, 2008
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 * $Id: XLock.c 396 2008-02-29 22:55:43Z jim $
 */


/**
 * @brief Cross-thread locking - this is the PTHREAD-callable version
 */

#include <pthread.h>

#include "platform/assert.h"

#include "sdfappcommon/XLock.h"
#include "XLock.h"

/**
 * @brief Get or wait for cross-thread lock (pthread callable only)
 *
 * @param cross <IN> Pointer to cross lock data structure
 * @param write <IN> Nonzero for write lock, zero for read lock
 */
void XLock(XLock_t *cross, int write) {
    // Increment the FTH lock to block FTH threads
    (void) __sync_fetch_and_add(&cross->fthLock, 1); // Increment FTH lock

    // Get and release the Q lock for queueing purposes only
    plat_assert_rc(pthread_rwlock_rdlock(&cross->qLock));
    plat_assert_rc(pthread_rwlock_unlock(&cross->qLock));

    // Release the FTH lock
    (void)__sync_fetch_and_sub(&cross->fthLock, 1);

    // Get the real lock (or queue for it)
    if (write) {
        plat_assert_rc(pthread_rwlock_wrlock(&cross->lock));
    } else {
        plat_assert_rc(pthread_rwlock_rdlock(&cross->lock));
    }
}

/**
 * @brief Try for cross-thread lock (pthread callable only)
 *
 * @param cross <IN> Pointer to cross lock data structure
 * @param write <IN> Nonzero for write lock, zero for read lock
 * @return int 0=OK; not zero=could not get lock
 */
int XTryLock(XLock_t *cross, int write) {
    // Increment the FTH lock to block FTH threads
    (void) __sync_fetch_and_add(&cross->fthLock, 1); // Increment FTH lock

    // Get and release the Q lock for queueing purposes only
    if (pthread_rwlock_tryrdlock(&cross->qLock) != 0) {
        (void) __sync_fetch_and_add(&cross->fthLock, -1);
        return 1;
    }
    plat_assert_rc(pthread_rwlock_unlock(&cross->qLock));

    // Release the FTH lock
    (void) __sync_fetch_and_sub(&cross->fthLock, 1);

    // Get the real lock (or queue for it)
    int rv;
    if (write) {
        rv = pthread_rwlock_trywrlock(&cross->lock);
    } else {
        rv = pthread_rwlock_tryrdlock(&cross->lock);
    }
    
    return (rv);
}

/**
 * @brief Release cross lock
 *
 * @param cross <IN> Pointer to cross lock data structure
 */
void XUnlock(XLock_t *cross) {
    plat_assert_rc(pthread_rwlock_unlock(&cross->lock));
}

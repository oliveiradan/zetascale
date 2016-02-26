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
 * File:   XLock.h
 * Author: Jim
 *
 * Created on February 29, 2008
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http: //www.schoonerinfotech.com/
 *
 * $Id: XLock.h 396 2008-02-29 22:55:43Z jim $
 */

//
//  Cross-threading (pthread/fth) lock
//

#ifndef _SDFAPP_COMMON_XLOCK_H
#define _SDFAPP_COMMON_XLOCK_H

#include <pthread.h>

#include "fth/fthLock.h"

typedef struct XLock {
    int fthLock;                             // Integer count
    pthread_rwlock_t qLock;                  // For queueing control
    pthread_rwlock_t lock;                   // Main lock
    int write;                               // Set for write lock
} XLock_t;

#endif

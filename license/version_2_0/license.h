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
 * Copyright (c) 2013 SanDisk Corporation.  All rights reserved.
 *
 * File:   license.h
 * Author: Niranjan Neelakanta
 *
 * Created on March 25, 2013.
 */

#ifndef SCHLICENSE_H
#define	SCHLICENSE_H
#include "license/interface.h"

//const char LICENSE_VER_TAG[] = "@@LICENSE@@ v1.0\n";
//const char CIPHERTEXT_TAG[] = "@@CIPHERTEXT@@\n";
//const char END_TAG[] = "\n@@END@@";


#ifdef __cplusplus
extern "C" {
#endif

/**
 * What is a Schooner license?<br>
 * The license has 2 parts to it - a text part and an encrypted part. <br>
 * (1) The text is created by an authority (Schooner web application or the
 * Sales team).<br>
 * (2) The encrypted part ("ciphertext") is generated by calling this method.<br>
 * What can be in the text part?<br>
 * (a) Any text that ends with '\0' and includes the following two lines:
 * "Date valid from:10/18/10 00:00:00\nDate valid to:01/22/11 00:00:00\n"
 * (b) The dates are fixed format "MM/DD/YY HH:MM:SS", spaces before and after
 * are valid.
 *
 * @param clear_license IN text containing from and to dates (see above)
 * @param clear_len IN length of the text
 * @param encr_len IN length of the buffer, this is to check if the creation of
 * the ciphertext may have buffer overflow
 * @param encr_license IN buffer to hold the generated ciphertext. As a fixed
 * current rule, this buffer should be 4X the size of the clear text.
 * @param message_out OUT a text buffer of atleast 256 char capacity to hold
 * messages provided back from the util to display or print in audit/debug logs.
 * @return 0 for success, other values on failure
 */
int GenerateLicense_v2_0(char *clear_license, unsigned clear_len,
        unsigned encr_len, char *encr_license, char *message_out);


/**
 * Check to see if the license provided in the arguments is valid. A license is
 * valid if the ciphertext (encrypted portion) is found valid && when the
 * current time (GMT) is between the begin and end date/time specified in the
 * license.<br>
 * For enhancements, checks can be put in either the licensed software or this
 * utility. The encryption will ensure that content of the text has not been
 * tampered with. Here are some samples:<br>
 * (1) In order to require a separate licenses for a different product, add
 * text "Schooner MemStore" to the text section, and check if it exists in
 * the software being licensed.<br>
 * (2) In order to require a separate license per node, add a field like
 * "Mac address: xxx-yyy-zzz" to the text section, and check it in the software
 * being licensed.<br>
 *
 * @param clear_license IN text containing from and to dates (see above)
 * @param encr_license IN buffer to hold the generated ciphertext.
 * @param message_out OUT a text buffer of atleast 256 char capacity to hold 
 * messages provided back from the util to display or print in audit/debug logs.
 * Even when license is valid, consider using this message as it display time
 * left in the license.
 * @return 0 for success, other values on failure
 */
#if 0
#ifdef __cplusplus
extern "C" 
#else
extern
#endif
#endif
//enum lic_state isLicenseValid(char *clear_license, char *encr_license,
//        char *message_out);

/**
 * See above for usage.
 *
 * @param license IN complete license string grabbed from a file and provided as
 * a C char* string.
 * @param product IN name of product (possibly with version number) to check.
 * @param check_mac_addr IN if non-zero, check for a MAC address 
 * provided in the license.
 * @param message_out OUT a text buffer of atleast 256 char capacity to hold
 * messages provided back from the util to display or print in audit/debug logs.
 * Even when license is valid, consider using this message as it display time
 * left in the license.
 * @return 0 for success, other values on failure
 */
#if 0
#ifdef __cplusplus
extern "C" 
#else
extern
#endif
#endif
enum lic_state isLicenseValid2_v2_0(char *clear_license, char *encr_license, char *product,

			char *version, int check_mac_addr, char *message_out);

#if 0
#ifdef __cplusplus
extern "C" 
#else
extern
#endif
#endif
enum lic_state verifyDates(char *, char *, char *);
int getTimeDiff(char *, char *, double *, double *);
enum lic_state mac_addr_matches(char *);
#ifdef __cplusplus
}
#endif
#endif	/* SCHLICENSE_H */

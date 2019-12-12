/**
 * @file
 * Media type interface.
 *
 * @copyright
 * @verbatim
   Copyright @ 2017 Audi Electronics Venture GmbH. All rights reserved.

       This Source Code Form is subject to the terms of the Mozilla
       Public License, v. 2.0. If a copy of the MPL was not distributed
       with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

   If it is not possible or desirable to put the notice in a particular file, then
   You may include the notice in a location (such as a LICENSE file in a
   relevant directory) where a recipient would be likely to look for such a notice.

   You may add additional accurate notices of copyright ownership.
   @endverbatim
 */



#ifndef IFHD_HEADER
#define IFHD_HEADER

#include <set>
#include <string>
#include <map>
#include <atomic>
#include <deque>
#include <list>
#include <utils5extension/utils5extension.h>

#ifndef DOEXPORT
    #define DOEXPORT  /* */
    #define DOEXPORT_STATIC /* */
    #define DEF_DOEXPORT
#endif

//*********************************************************************************************
#ifdef OPT_INLINE
    #undef OPT_INLINE
#endif // OPT_INLINE

#ifdef UCOM_OPT_INLINE
    #define OPT_INLINE inline
#else
    #define OPT_INLINE  /* */
#endif // UCOM_OPT_INLINE
/**
 * @def OPT_INLINE
 * the compiler will insert the code inline. in this case the execution code is a greater, but the processor doesnt need to jump into the function
 */

#include "indexedfile_pkg.h"

#ifdef DEF_DOEXPORT
    #undef DOEXPORT
    #undef DEF_DOEXPORT
#endif

#endif //_IFHD_HEADER_

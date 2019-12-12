/**
 * @file
 * Utils 5 Extension
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



#ifndef UTILS5_EXT_HEADER
#define UTILS5_EXT_HEADER

#ifdef min
    #undef min
#endif
#ifdef max
    #undef max
#endif

#include <a_util/concurrency.h>
#include <a_util/filesystem.h>
#include <a_util/datetime.h>
#include <a_util/memory.h>
#include <a_util/xml.h>
#include <limits>
#include <queue>

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

#include "utils5ext_pkg.h"

#ifdef DEF_DOEXPORT
    #undef DOEXPORT
    #undef DEF_DOEXPORT
#endif

#endif //_UTILS5EXT_HEADER_

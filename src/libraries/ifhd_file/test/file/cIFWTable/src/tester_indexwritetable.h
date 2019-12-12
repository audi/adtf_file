/**
 * @file
 * Tester init header.
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


#ifndef TESTER_INDEXTABLE_HEADER
#define TESTER_INDEXTABLE_HEADER


DECLARE_TESTER_CLASS(TesterIndexWriteTable)
{

    BEGIN_TESTER_MAP(TesterIndexWriteTable)
        DECLARE_TESTER_FUNCTION(TestInit)
        DECLARE_TESTER_FUNCTION(TestBuffer)
        DECLARE_TESTER_FUNCTION(TestAppend)
    END_TESTER_MAP()


public:
    //Definition of test methods
    tTesvoid TestInit();
    tTesvoid TestBuffer();
    tTesvoid TestAppend();
};

#endif // _TESTER_INDEXEDFILEREADER_HEADER_

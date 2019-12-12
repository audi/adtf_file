/**
 * @file
 * Test setup header
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


#include "a_util/filesystem.h"
#include "gtest/gtest.h"

//same as macro expansion of GTEST_TEST_CLASS_NAME_() with a '_Req' added
#define TEST_REQ_CLASS_NAME_(test_case_name, test_name) test_case_name##_##test_name##_Test_Req

//create the parent class for setup of the requirement as additional property
#define TEST_REQ_FIXTURE_(test_case_name, test_name, req_id)                        \
    class TEST_REQ_CLASS_NAME_(test_case_name, test_name) : public ::testing::Test  \
    {                                                                               \
        protected:                                                                  \
        virtual void SetUp()                                                        \
        {                                                                           \
            Test::RecordProperty("requirement", req_id);                            \
            Test::SetUp();                                                          \
        }                                                                           \
        virtual void TearDown()                                                     \
        {                                                                           \
            Test::TearDown();                                                       \
        }                                                                           \
    };

//create the requirement test fixture class
#define TEST_REQ(test_case_name, test_name, req_id)                                 \
    TEST_REQ_FIXTURE_(test_case_name, test_name, req_id)                            \
    TEST_F(TEST_REQ_CLASS_NAME_(test_case_name, test_name), test_name)

int main(int argc, char **argv)
{   
    // the working directory of tests should be the source directory,
    // to be able to access test files easily
    a_util::filesystem::Path source_path = __FILE__;
    source_path.RemoveLastElement();
    if (source_path.GetLastElement() == "test")
    {
        source_path.RemoveLastElement();
    }

    if (!source_path.IsEmpty() && source_path.IsAbsolute())
    {
        a_util::filesystem::SetWorkingDirectory(source_path);
    }

    // now setup gtest
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

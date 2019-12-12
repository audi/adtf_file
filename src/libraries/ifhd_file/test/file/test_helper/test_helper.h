/**
 * @file
 * Test helper
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

#ifndef TEST_IFHD_HELPER
#define TEST_IFHD_HELPER

#define A_UTILS_TEST(expression) ASSERT_TRUE(expression)
#define A_UTILS_TEST_ERR(expression) ASSERT_FALSE(expression)
#define A_UTILS_TEST_EXT(expression, errordesc) ASSERT_TRUE(expression)
#define A_UTILS_TEST_RESULT(expression) ASSERT_NO_THROW(expression)
#define A_UTILS_TEST_ERR_RESULT(expression) ASSERT_ANY_THROW(expression)
#define A_UTILS_TEST_RESULT_EXT(expression, errordesc) ASSERT_NO_THROW(expression)
#define A_UTILS_TEST_ERR_RESULT_EXT(expression, errordesc) ASSERT_ANY_THROW(expression)

#define DEFINE_TEST(tester_class, tester_fc, val1, val2, val3, val4, val5, val6, val7, val8) GTEST_TEST(tester_fc, tester_class)
#endif TEST_IFHD_HELPER

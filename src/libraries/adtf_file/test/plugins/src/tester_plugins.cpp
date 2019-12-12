/**
 * @file
 * Tester init.
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

#include "gtest/gtest.h"
#include <adtf_file/object.h>
#include <adtf_file/adtf_file_reader.h>

using namespace adtf_file;

adtf_file::Objects objects;

GTEST_TEST(TestPlugins, ObjectsFromPlugin)
{    
    adtf_file::loadPlugin(TEST_PLUGIN);

    Reader reader(TEST_FILES_DIR "/test_stop_signal.dat",
                  adtf_file::getFactories<adtf_file::StreamTypeDeserializers, adtf_file::StreamTypeDeserializer>(),
                  adtf_file::getFactories<adtf_file::SampleDeserializerFactories, adtf_file::SampleDeserializerFactory>());
}

std::string load_unsupported_plugin(const std::string& plugin_file_name)
{
    try
    {
        adtf_file::loadPlugin(plugin_file_name);
    }
    catch (const std::exception& error)
    {
        return error.what();
    }

    return "";
}

GTEST_TEST(TestOldPlugins, ObjectsFromPlugin)
{
    ASSERT_NE(std::string::npos, load_unsupported_plugin(TEST_PLUGIN_OLD_NO_VERSION).find("0.5.0 or less"));
    ASSERT_NE(std::string::npos, load_unsupported_plugin(TEST_PLUGIN_OLD).find("very old"));
}




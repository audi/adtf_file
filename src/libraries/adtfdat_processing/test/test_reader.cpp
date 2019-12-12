#include <gtest/gtest.h>
#include <adtfdat_processing/reader.h>
#include "test_reader.h"

using namespace adtf::dat;

GTEST_TEST(Reader, findStream)
{
    TestReader<0> reader;
    ASSERT_EQ(findStream(reader, "stream2").name, "stream2");
    ASSERT_ANY_THROW(findStream(reader, "stream3"));
}

GTEST_TEST(ReaderFactory, make)
{
    ReaderFactoryImplementation<TestReader<0>> factory;
    auto reader = factory.make();
    ASSERT_EQ(reader->getReaderIdentifier(), "test_0");
}

GTEST_TEST(ReaderFactories, make)
{
    ReaderFactories factories;
    factories.add(std::make_shared<ReaderFactoryImplementation<TestReader<0>>>());
    factories.add(std::make_shared<ReaderFactoryImplementation<TestReader<1, true>>>());

    ASSERT_EQ(factories.make("test_0")->getReaderIdentifier(), "test_0");
    ASSERT_EQ(factories.make("test_1")->getReaderIdentifier(), "test_1");

    auto capable_readers = factories.getCapableReaders("compatible");
    ASSERT_EQ(capable_readers.size(), 1);
    ASSERT_EQ(capable_readers.count("test_0"), 1);

    ASSERT_ANY_THROW(factories.getCapableReaders("not compatible"));
}

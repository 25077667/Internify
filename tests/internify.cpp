#include <gtest/gtest.h>
#include <internify.hpp>

#include <thread>
#include <vector>

TEST(InternifyTest, BasicUsage)
{
    scc::Internify<std::string> intern;

    auto str1 = intern.internify("hello");
    auto str2 = intern.internify("hello");
    auto str3 = intern.internify("world");

    EXPECT_EQ(str1, str2);
    EXPECT_NE(str1, str3);
    EXPECT_EQ(*str1, "hello");
    EXPECT_EQ(*str3, "world");
}

TEST(InternifyTest, NoResourceLeakage)
{
    scc::Internify<std::string> intern;

    auto str1 = intern.internify("leaktest");
    auto str2 = intern.internify("leaktest");

    EXPECT_EQ(intern.size(), 1);

    intern.release("leaktest");
    intern.release("leaktest");

    EXPECT_EQ(intern.size(), 0);
}

TEST(InternifyTest, EdgeCases)
{
    scc::Internify<std::string> intern;

    // Test empty string
    auto emptyStr1 = intern.internify("");
    auto emptyStr2 = intern.internify("");
    EXPECT_EQ(emptyStr1, emptyStr2);
    EXPECT_EQ(*emptyStr1, "");
    intern.release("");
    intern.release(""); // we should erase twice, because we interned twice

    // Test large number of strings
    const int numStrings = 10000;
    std::vector<std::shared_ptr<std::string>> strings;
    for (int i = 0; i < numStrings; ++i)
    {
        strings.push_back(intern.internify("test" + std::to_string(i)));
    }

    EXPECT_EQ(intern.size(), numStrings);
}

TEST(InternifyTest, Robustness)
{
    scc::Internify<std::string> intern;

    const int numStrings = 10000;
    std::vector<std::shared_ptr<std::string>> strings;

    for (int i = 0; i < numStrings; ++i)
    {
        strings.push_back(intern.internify("robust" + std::to_string(i)));
    }

    for (int i = 0; i < numStrings; ++i)
    {
        intern.release("robust" + std::to_string(i));
    }

    EXPECT_EQ(intern.size(), 0);
}

TEST(InternifyTest, ThreadSafety)
{
    scc::Internify<std::string> intern;

    const int numThreads = 10;
    const int numOperations = 1000;

    auto internifyFunction = [&intern](int id)
    {
        for (int i = 0; i < numOperations; ++i)
        {
            std::string str = "threadsafe" + std::to_string(i + id * numOperations);
            intern.internify(str);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(internifyFunction, i);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    EXPECT_EQ(intern.size(), numThreads * numOperations);

    // Ensure no deadlock or crash
    for (int i = 0; i < numThreads; ++i)
    {
        for (int j = 0; j < numOperations; ++j)
        {
            std::string str = "threadsafe" + std::to_string(j + i * numOperations);
            intern.release(str);
        }
    }

    EXPECT_EQ(intern.size(), 0);
}

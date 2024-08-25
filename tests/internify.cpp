#include <gtest/gtest.h>
#include <internify.hpp>

#include <thread>
#include <vector>
#include <mutex>

TEST(InternifyTest, BasicUsage)
{
    scc::Internify<std::string> intern;

    auto str1 = intern.internify("hello");
    auto str2 = intern.internify("hello");
    auto str3 = intern.internify("world");

    EXPECT_EQ(str1.get(), str2.get()); // InternedPtr should point to the same instance
    EXPECT_NE(str1.get(), str3.get()); // InternedPtr should point to different instances
    EXPECT_EQ(*str1, "hello");
    EXPECT_EQ(*str3, "world");
}

TEST(InternifyTest, NoResourceLeakage)
{
    scc::Internify<std::string> intern;

    {
        auto str1 = intern.internify("leaktest");
        auto str2 = intern.internify("leaktest");

        EXPECT_EQ(intern.size(), 1);
    } // str1 and str2 go out of scope here

    EXPECT_EQ(intern.size(), 0); // InterningNode should be released automatically
}

TEST(InternifyTest, EdgeCases)
{
    scc::Internify<std::string> intern;

    // Test empty string
    {
        auto emptyStr1 = intern.internify("");
        auto emptyStr2 = intern.internify("");

        EXPECT_EQ(emptyStr1.get(), emptyStr2.get()); // InternedPtr should point to the same instance
        EXPECT_EQ(*emptyStr1, "");
    }

    // Test large number of strings
    const int numStrings = 10000;
    std::vector<scc::Internify<std::string>::InternedPtr> strings;
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
    std::vector<scc::Internify<std::string>::InternedPtr> strings;

    for (int i = 0; i < numStrings; ++i)
    {
        strings.push_back(intern.internify("robust" + std::to_string(i)));
    }

    for (int i = 0; i < numStrings; ++i)
    {
        intern.erase("robust" + std::to_string(i));
    }

    EXPECT_EQ(intern.size(), 0);
}

TEST(InternifyTest, ThreadSafety)
{
    scc::Internify<std::string> intern;
    std::vector<scc::Internify<std::string>::InternedPtr> internedStrings;

    const int numThreads = 10;
    const int numOperations = 1000;

    auto internifyFunction = [&intern, &internedStrings](int id)
    {
        for (int i = 0; i < numOperations; ++i)
        {
            std::string str = "threadsafe" + std::to_string(i + id * numOperations);
            static std::mutex mutex;
            std::lock_guard lock(mutex);
            internedStrings.emplace_back(intern.internify(str));
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
            intern.erase(str); // Directly erase instead of relying on InternedPtr release
        }
    }

    EXPECT_EQ(intern.size(), 0);
}

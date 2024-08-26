# 🌟 SCC Internify Library

## Overview

The `SCC Internify` library is a lightweight, thread-safe C++17 library designed for interning strings (or any hashable type). Interning is a technique that optimizes memory usage by ensuring that only one copy of each unique value is stored and shared across your application. This can significantly reduce memory overhead and improve performance, especially in scenarios where many identical strings or objects are used repeatedly.

## Features
- **🔒 Thread-Safe Interning**: Safely intern and share objects across multiple threads without data races. The `scc::Internify` class uses a combination of `std::shared_mutex` for concurrent read access and `std::unique_lock` for write access, ensuring safe multi-threaded operation.
- **⚙️ Customizable Hashing**: Easily provide your own hash function, or use the default `std::hash<T>`. The `scc::Internify` class template allows you to specify a custom hash function through the `HashFunc` template parameter.
- **🧠 Automatic Reference Counting**: Interned objects are automatically managed using a custom reference counting mechanism, ensuring that each object is properly cleaned up when no longer in use. No need for `std::shared_ptr`, leading to a lighter and more efficient implementation.
- **⚡ Efficient Memory Usage**: Only a single instance of each unique object is stored, minimizing memory usage. Interning helps reduce the overhead of storing multiple identical objects, leading to better resource utilization.
- **🗜️ C++17, Single Header, Zero Dependencies**: Just include the `internify.hpp` header, and you're ready to go—no external dependencies required. The class is implemented entirely within a header file, making it easy to integrate into existing projects.
- **📜 MIT License**: Free for both personal and commercial use. The `scc::Internify` class is released under the MIT license, making it easy to use in both open-source and proprietary projects.

## Installation

To use the `SCC Internify` library, simply include the header file in your project:

```cpp
#include <internify.hpp>
```

There’s no need for additional setup or dependencies—just drop the header file into your project.

## Usage Example

Here’s a simple example of how to use the `Internify` class, demonstrated through Google Test unit tests:

```cpp
TEST(InternifyTest, BasicUsage)
{
    scc::Internify<std::string> intern;

    auto str1 = intern.internify("hello");  // return type is InternedPtr<std::string, std::hash<std::string>>
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

// Additional tests omitted for brevity...
```

## Running Tests

The `SCC Internify` library includes a comprehensive set of tests that are automatically handled by CMake. To build and run the tests:

1. **CMake Configuration**: The CMake build system is set up to automatically fetch Google Test and Google Benchmark as dependencies. Tests are discovered and built automatically unless you define `INTERNIFY_DISABLE_TESTS` to disable the test build.

2. **Build and Run**: Simply configure and build your project with CMake as usual, and the tests will be compiled. You can then run the tests using `ctest` or directly from the build directory.

```bash
mkdir build
cd build
cmake ..
make
ctest
```

### CMake Details

Here’s a brief overview of how the CMake script handles tests:

- **Test Control**: If `INTERNIFY_DISABLE_TESTS` is defined, tests will not be built.
- **Fetching Dependencies**: The script fetches Google Test and Google Benchmark from their respective repositories.
- **Test Discovery**: All test sources in the `tests` folder are automatically discovered, compiled, and linked against Google Test.
- **Test Execution**: Tests are executed and discovered via `gtest_discover_tests`, which integrates seamlessly with CTest.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing

Contributions are welcome! If you encounter any issues or have ideas for improvements, please open an issue or submit a pull request on GitHub.

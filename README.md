# 🌟 SCC Internify Library

## Overview

The `SCC Internify` library is a lightweight, thread-safe C++17 library designed for interning strings (or any hashable type). Interning is a technique that optimizes memory usage by ensuring that only one copy of each unique value is stored and shared across your application. This can significantly reduce memory overhead and improve performance, especially in scenarios where many identical strings or objects are used repeatedly.

## Features

- **🔒 Thread-Safe Interning**: Safely intern and share objects across multiple threads without data races.
- **⚙️ Customizable Hashing**: Easily provide your own hash function, or use the default `std::hash<T>`.
- **🧠 Automatic Memory Management**: Interned objects are managed using `std::shared_ptr`, ensuring proper cleanup.
- **⚡ Efficient Resource Handling**: Only a single instance of each unique object is stored, minimizing memory usage.
- **🗜️ C++17, Single Header, Zero Dependencies**: Just include the header and you're ready to go—no external dependencies required.
- **📜 MIT License**: Free for both personal and commercial use.

## Installation

To use the `SCC Internify` library, simply include the header file in your project:

```cpp
#include <internify.hpp>
```

There’s no need for additional setup or dependencies—just drop the header file into your project.

## Usage Example

Here’s a simple example of how to use the `Internify` class, demonstrated through Google Test unit tests:

```cpp
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

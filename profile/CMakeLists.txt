set(PROFILE_SRC driver.cpp)

add_executable(profile_driver ${PROFILE_SRC})
target_compile_options(profile_driver PRIVATE -g)

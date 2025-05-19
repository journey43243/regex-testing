cmake_minimum_required(VERSION 3.18)
project(MyProject)


cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0144 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(MSYS2_PREFIX "C:/msys64/mingw64")
list(APPEND CMAKE_PREFIX_PATH "${MSYS2_PREFIX}")


set(BOOST_ROOT "${MSYS2_PREFIX}")
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)


find_package(Boost 1.86.0 REQUIRED COMPONENTS regex)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCRE REQUIRED libpcre2-8)

set(re2_DIR "${MSYS2_PREFIX}/lib/cmake/re2")
set(absl_DIR "${MSYS2_PREFIX}/lib/cmake/absl")

find_library(RE2_LIB re2 PATHS "${MSYS2_PREFIX}/lib" REQUIRED)
find_path(RE2_INCLUDE_DIR re2/re2.h PATHS "${MSYS2_PREFIX}/include" REQUIRED)

file(GLOB ABSL_LIBS "${MSYS2_PREFIX}/lib/libabsl_*.a")
find_path(ABSL_INCLUDE_DIR absl/strings/str_cat.h PATHS "${MSYS2_PREFIX}/include" REQUIRED)
add_executable(MyProject main.cpp)

if(NOT Boost_USE_STATIC_LIBS)
    file(GLOB ABSL_DLLS "${MSYS2_PREFIX}/bin/libabsl_*.dll")
    foreach(dll ${ABSL_DLLS})
        add_custom_command(TARGET MyProject POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                $<TARGET_FILE_DIR:MyProject>
        )
    endforeach()

    add_custom_command(TARGET MyProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${MSYS2_PREFIX}/bin/libpcre2-8-0.dll"
            $<TARGET_FILE_DIR:MyProject>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MSYS2_PREFIX}/bin/libre2-11.dll"
            $<TARGET_FILE_DIR:MyProject>
    )
endif()

target_link_libraries(MyProject
    PRIVATE
        Boost::regex
        ${PCRE_LIBRARIES}
        ${RE2_LIB}
        ${ABSL_LIBS}
)

target_include_directories(MyProject
    PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${PCRE_INCLUDE_DIRS}
        ${re2_INCLUDE_DIRS}
        ${absl_INCLUDE_DIRS}
)

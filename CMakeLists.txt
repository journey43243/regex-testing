cmake_minimum_required(VERSION 3.18)
project(MyProject)

# Установка политик CMake
cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0144 NEW)  # Для правильного использования BOOST_ROOT

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к Boost (для MSYS2/MinGW)
set(BOOST_ROOT "C:/msys64/mingw64")
set(Boost_NO_SYSTEM_PATHS ON)  # Запрещаем поиск в системных путях
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Добавляем путь к библиотекам Boost в CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "${BOOST_ROOT}")

# Поиск Boost с компонентом regex
find_package(Boost 1.86.0 REQUIRED COMPONENTS regex)

# Поиск PCRE (если нужно)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCRE REQUIRED libpcre2-8)

add_executable(MyProject main.cpp)

add_custom_command(TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "C:/msys64/mingw64/bin/libpcre2-8-0.dll"
        $<TARGET_FILE_DIR:MyProject>
)

# Подключение библиотек
target_link_libraries(MyProject
    PRIVATE
        Boost::regex
        ${PCRE_LIBRARIES}
)
target_include_directories(MyProject
    PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${PCRE_INCLUDE_DIRS}
)
# Добавление include-директорий
target_include_directories(MyProject
    PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${PCRE_INCLUDE_DIRS}
)
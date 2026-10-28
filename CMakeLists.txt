cmake_minimum_required(VERSION 3.16...4.4)

if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(FATAL_ERROR
        "C++ modules require 'Ninja' or 'Visual Studio' generator.\n"
    )
endif()

project(asapage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# lib
function(build_vendor_lib lib_name out_cmake_dir)
    get_filename_component(VENDOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${lib_name}" ABSOLUTE)
    get_filename_component(VENDOR_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/_out/${lib_name}" ABSOLUTE)

    set(VENDOR_BUILD_DIR "${VENDOR_OUT_DIR}/build")
    set(VENDOR_INSTALL_DIR "${VENDOR_OUT_DIR}/install")

    if(NOT EXISTS "${VENDOR_BUILD_DIR}/CMakeCache.txt")
        message(STATUS "Building vendor: ${lib_name}...")

        file(TO_NATIVE_PATH "${VENDOR_SOURCE_DIR}" NATIVE_SOURCE_DIR)
        file(TO_NATIVE_PATH "${VENDOR_BUILD_DIR}" NATIVE_BUILD_DIR)
        file(TO_NATIVE_PATH "${VENDOR_INSTALL_DIR}" NATIVE_INSTALL_DIR)

        # config
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S "${NATIVE_SOURCE_DIR}"
                -B "${NATIVE_BUILD_DIR}"
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX="${NATIVE_INSTALL_DIR}"
                ${ARGN}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE CONFIG_RESULT
        )

        if(NOT CONFIG_RESULT EQUAL 0)
            file(REMOVE_RECURSE "${VENDOR_OUT_DIR}")
            message(FATAL_ERROR "Failed to configure vendor target: ${lib_name}")
        endif()

        # build
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build "${NATIVE_BUILD_DIR}" --config Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE BUILD_RESULT
        )

        if(NOT BUILD_RESULT EQUAL 0)
            file(REMOVE_RECURSE "${VENDOR_OUT_DIR}")
            message(FATAL_ERROR "Failed to build vendor target: ${lib_name}")
        endif()

        # install
        execute_process(
            COMMAND ${CMAKE_COMMAND} --install "${NATIVE_BUILD_DIR}" --prefix "${NATIVE_INSTALL_DIR}" --config Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE INSTALL_RESULT
        )

        if(NOT INSTALL_RESULT EQUAL 0)
            file(REMOVE_RECURSE "${VENDOR_OUT_DIR}")
            message(FATAL_ERROR "Failed to install vendor target: ${lib_name}")
        endif()
    else()
        message(STATUS "OK, ${lib_name}")
    endif()

    if(EXISTS "${VENDOR_INSTALL_DIR}/lib64/cmake/${lib_name}")
        set(${out_cmake_dir} "${VENDOR_INSTALL_DIR}/lib64/cmake/${lib_name}" PARENT_SCOPE)
    else()
        set(${out_cmake_dir} "${VENDOR_INSTALL_DIR}/lib/cmake/${lib_name}" PARENT_SCOPE)
    endif()
endfunction()

build_vendor_lib(raylib RAYLIB_CMAKE_DIR)
find_package(raylib REQUIRED CONFIG
    PATHS "${RAYLIB_CMAKE_DIR}"
    NO_DEFAULT_PATH
)

# var
file(GLOB_RECURSE ALL_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

get_filename_component(MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" ABSOLUTE)
list(REMOVE_ITEM ALL_SOURCES "${MAIN_SOURCE}")

# target
add_executable(${PROJECT_NAME} "${MAIN_SOURCE}")

target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES ${ALL_SOURCES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(ENABLE_ASAN)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address /experimental:module-)
        target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    endif()
endif()

# include
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# link
target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
)

if (WIN32)
    target_link_libraries(asapage PRIVATE dbghelp)
endif()

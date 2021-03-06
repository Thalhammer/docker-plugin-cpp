cmake_minimum_required(VERSION 3.13)
project(DockerPluginCPP)
cmake_policy(SET CMP0077 NEW) # Ignore normal variables in option
enable_testing()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

option(BUILD_STATIC "Build statically" ON)
option(BUILD_FULL_STATIC "Build fully static" OFF)
option(WITH_ASAN "Enable asan builds" OFF)
option(BUILD_SAMPLES "Enable test builds" ON)

# Enable Link-Time Optimization
if(NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTOAvailable)
else()
	add_compile_options("-ggdb3")
endif()
if(LTOAvailable)
    message(STATUS "Link-time optimization enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
if(BUILD_STATIC)
    message(STATUS "Building static binary")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    if(BUILD_FULL_STATIC)
    	message(STATUS "Building fully static binary")
        set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
        set(CMAKE_LINK_SEARCH_START_STATIC 1)
        set(CMAKE_LINK_SEARCH_END_STATIC 1)
    endif()
endif()
if(WITH_ASAN)
    message(STATUS "Building with asan enabled")
    add_compile_options(-fsanitize=address)
	link_libraries(asan)
endif()

message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")

add_subdirectory(lib)
if(BUILD_SAMPLES)
    add_subdirectory(sample_volume)
endif()
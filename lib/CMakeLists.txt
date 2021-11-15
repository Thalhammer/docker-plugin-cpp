find_package(Threads REQUIRED)
include(FetchContent)
FetchContent_Declare(llhttp URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v6.0.5.zip")  # Using version 6.0.5
FetchContent_MakeAvailable(llhttp)

add_library(docker-plugin-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uds_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serialize.cpp
)
target_link_libraries(docker-plugin-cpp PRIVATE llhttp Threads::Threads)
target_include_directories(docker-plugin-cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(docker-plugin-cpp PUBLIC cxx_std_11)
target_compile_options(docker-plugin-cpp PRIVATE -Wall -Wextra -Werror -Weffc++ -Wold-style-cast)
target_compile_definitions(docker-plugin-cpp PRIVATE -DPICOJSON_USE_INT64)
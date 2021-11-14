add_executable(sample_volume
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
target_include_directories(sample_volume PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sample_volume PRIVATE docker-plugin-cpp)
target_compile_options(sample_volume PRIVATE -Wall -Wextra -Werror -Weffc++ -Wold-style-cast)

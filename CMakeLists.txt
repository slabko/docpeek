cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 20)

set(BOOST_INCLUDE_LIBRARIES thread filesystem system program_options asio date_time)
set(BOOST_ENABLE_CMAKE ON)

project(docpeek)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)
# add_compile_options(-fsanitize=undefined)
# add_link_options(-fsanitize=undefined)
# 
# # compiler sometimes fail to optimize loops with sanitizers enabled
# add_compile_options(-Wno-pass-failed)
# 
# add_compile_options(
#     -march=native 
#     -Wall 
#     -Wno-unused 
#     -Wextra 
#     -Wconversion 
#     -pedantic 
#     -Werror 
#     )

find_package(fmt REQUIRED)
find_package(nlohmann_json 3.10.0 REQUIRED)

add_executable(docpeek_exec
    src/main.cpp
    )

target_compile_options(docpeek_exec PRIVATE
    -Wall 
    -Wextra 
    -Wconversion 
    -pedantic 
    -Werror 
    )

target_link_libraries(docpeek_exec PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    )


set_target_properties(docpeek_exec PROPERTIES 
    OUTPUT_NAME "docpeek"
    )

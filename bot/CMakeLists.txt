cmake_minimum_required(VERSION 3.26)

include(FetchContent)
FetchContent_Declare(
    jsmn
    GIT_REPOSITORY https://github.com/zserge/jsmn.git
    GIT_TAG master
)
FetchContent_MakeAvailable(jsmn)

find_package(OpenSSL REQUIRED)

set(SOURCES
    src/bot.c
)

set(HEADERS
)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

add_executable(bot ${SOURCES} ${HEADERS})

target_include_directories(bot
    PUBLIC
        include
)

set_target_properties(bot PROPERTIES
    C_STANDARD 17
)

target_link_libraries(bot discord)
target_link_libraries(bot OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(bot PRIVATE ${jsmn_SOURCE_DIR})

target_link_options(bot PRIVATE -rdynamic)
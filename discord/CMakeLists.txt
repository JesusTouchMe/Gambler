cmake_minimum_required(VERSION 3.26)

find_package(OpenSSL REQUIRED)

set(SOURCES
    src/discord.c
    src/utils/webutils.c
    src/jsmn.c
    src/utils/jsonutils.c
    src/internal/memory.c
        src/discord/types.c
        ../launchwrapper/src/main.c
        src/discord/message.c
        src/discord/api.c
        src/utils/time.c
        src/discord/events.c
)

set(HEADERS
    include/discord.h
    include/utils/webutils.h
        include/discord/intents.h
    include/utils/jsonutils.h
    include/internal/memory.h
    include/discord/types.h
        include/discord/message.h
        include/discord/api.h
        include/discord/function_types.h
        include/utils/time.h
        include/discord/events.h
)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

add_library(discord STATIC ${SOURCES} ${HEADERS})

target_include_directories(discord
    PUBLIC
        include
)

set_target_properties(discord PROPERTIES
    C_STANDARD 17
)

target_link_libraries(discord OpenSSL::SSL OpenSSL::Crypto)
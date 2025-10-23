cmake_minimum_required(VERSION 3.26)

set(SOURCES
        src/main.c
)

set(HEADERS

)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

add_library(launchwrapper STATIC ${SOURCES} ${HEADERS})

target_include_directories(launchwrapper
        PUBLIC
            include
)

set_target_properties(launchwrapper PROPERTIES
        C_STANDARD 17
)

target_link_libraries(launchwrapper discord)
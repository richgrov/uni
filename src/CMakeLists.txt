set(UNI_SOURCES
    net/uni_connection.h
    net/uni_networking.h
    protocol/uni_handshake.c
    protocol/uni_handshake.h
    protocol/uni_packet.c
    protocol/uni_packet.h
    uni.c
    uni_log.h
    uni_os_constants.h
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(UNI_SOURCES ${UNI_SOURCES}
        net/uni_uring.c
        net/uni_uring.h
    )
endif()

add_library(uni ${UNI_SOURCES})

target_include_directories(uni PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(uni PRIVATE "${PROJECT_SOURCE_DIR}/src")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_command(TARGET uni
        PRE_LINK
        COMMAND "make"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/deps/liburing"
    )

    target_link_libraries(uni PRIVATE "${PROJECT_SOURCE_DIR}/deps/liburing/src/liburing.a")
    target_include_directories(uni PRIVATE "${PROJECT_SOURCE_DIR}/deps/liburing/src/include")
endif()

# Will define UNI_BIG_ENDIAN if the operating system is big-endian encoding
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    target_compile_definitions(uni PRIVATE UNI_BIG_ENDIAN)
endif()

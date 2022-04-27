set(UNI_SOURCES
    net/uni_connection.h
    net/uni_networking.h
    protocol/uni_packet.c
    protocol/uni_packet.h
    protocol/uni_packet_handler.c
    protocol/uni_packet_handler.h
    uni.c
    uni_log.h
    uni_os_constants.h
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(UNI_SOURCES ${UNI_SOURCES} net/uni_uring.c)
endif()

add_library(uni ${UNI_SOURCES})

target_include_directories(uni PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(uni PRIVATE "${PROJECT_SOURCE_DIR}/src")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
        COMMAND "make"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/deps/liburing"
    )

    target_link_libraries(uni PRIVATE "${PROJECT_SOURCE_DIR}/deps/liburing/src/liburing.a")
    target_include_directories(uni PRIVATE "${PROJECT_SOURCE_DIR}/deps/liburing/src/include")
endif()

# Defines UNI_DEBUG if this is a debug build
target_compile_definitions(uni PRIVATE "$<$<CONFIG:DEBUG>:UNI_DEBUG>")

# Will define UNI_BIG_ENDIAN if the operating system is big-endian encoding
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    target_compile_definitions(uni PRIVATE UNI_BIG_ENDIAN)
endif()

# hmac_sha256
add_subdirectory("../deps/hmac_sha256" "build_hmac_sha256")
target_link_libraries(uni PRIVATE hmac_sha256)

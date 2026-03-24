# Scheduler Service Model CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

add_subdirectory(MESSAGES)

# Scheduler SM source files
set(SCHEDULER_SM_SOURCES
    scheduler_sm.c
    scheduler_enc.c
    scheduler_dec.c
)

# Create scheduler SM library
add_library(scheduler_sm STATIC ${SCHEDULER_SM_SOURCES})

# Include directories
target_include_directories(scheduler_sm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/MESSAGES  # ASN.1 generated headers
    ${CMAKE_SOURCE_DIR}/common/utils/LOG
    ${CMAKE_SOURCE_DIR}/openair2/E3AP
)

target_link_libraries(scheduler_sm scheduler_sm_asn1)
add_dependencies(scheduler_sm scheduler_sm_asn1)

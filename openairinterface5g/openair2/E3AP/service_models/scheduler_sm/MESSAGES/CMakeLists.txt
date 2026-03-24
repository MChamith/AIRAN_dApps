set(SCHEDULER_SM_VERSION 1 0 0)
make_version(SCHEDULER_SM_cc ${SCHEDULER_SM_VERSION})
string(REPLACE ";" "." SCHEDULER_SM_RELEASE "${SCHEDULER_SM_VERSION}")

if(SCHEDULER_SM_RELEASE VERSION_EQUAL "1.0.0")
  include(ASN1/V1/e3sm_scheduler.cmake)
else()
  message(FATAL_ERROR "unknown SCHEDULER_SM_RELEASE ${SCHEDULER_SM_RELEASE}")
endif()

message(STATUS "Selected SCHEDULER_SM_VERSION: ${SCHEDULER_SM_RELEASE}")
add_custom_command(OUTPUT ${scheduler_sm_source} ${scheduler_sm_headers}
  COMMAND ${ASN1C_EXEC} -pdu=all -gen-APER -gen-UPER -no-gen-JER -no-gen-BER -no-gen-OER -fno-include-deps -fcompound-names -findirect-choice -no-gen-example -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEDULER_SM_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEDULER_SM_GRAMMAR}
  COMMENT "Generating Scheduler SM source files from ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEDULER_SM_GRAMMAR}"
)

add_library(scheduler_sm_asn1 ${scheduler_sm_source})
target_include_directories(scheduler_sm_asn1 PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(scheduler_sm_asn1 PRIVATE -DASN_DISABLE_OER_SUPPORT -w)

# Export variables to parent scope
set(SCHEDULER_SM_ASN1_LIB scheduler_sm_asn1 PARENT_SCOPE)
set(SCHEDULER_SM_ASN1_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

cmake_minimum_required(VERSION 3.16)
# Board profile is build-time only. Do not edit pins in a running device.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(RFLINK_BOARD_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/boards/rflink_s3_n16r8.json")
set(RFLINK_BOARD_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/components/rflink_radio_hal/include/rflink_board_generated.hpp")
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_board_header.py"
    --input "${RFLINK_BOARD_CONFIG}" --output "${RFLINK_BOARD_HEADER}" --print-hash
  OUTPUT_VARIABLE RF_BOARD_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RF_BOARD_RESULT
  ERROR_VARIABLE RF_BOARD_ERROR)
string(LENGTH "${RF_BOARD_HASH}" RF_BOARD_HASH_LENGTH)
if(NOT RF_BOARD_RESULT EQUAL 0 OR NOT RF_BOARD_HASH MATCHES "^[0-9a-f]+$" OR NOT RF_BOARD_HASH_LENGTH EQUAL 8)
  message(FATAL_ERROR "RFLink board profile invalid: ${RF_BOARD_ERROR}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RFLINK_BOARD_CONFIG}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_board_header.py")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Default builds remain explicitly identified as local development builds.
# An immutable GitHub firmware release supplies its tag through the Builder,
# without editing the source or shifting the original release tag.
set(PROJECT_VER "0.7.0-n16r8-s20-b${RF_BOARD_HASH}")
if(DEFINED ENV{RFLINK_RELEASE_TAG} AND NOT "$ENV{RFLINK_RELEASE_TAG}" STREQUAL "")
  set(RFLINK_TAG "$ENV{RFLINK_RELEASE_TAG}")
  if(RFLINK_TAG MATCHES "^firmware/v0[.]7[.]0-exp[.]([1-9][0-9]*)$")
    set(PROJECT_VER "0.7.0-exp.${CMAKE_MATCH_1}-n16r8-b${RF_BOARD_HASH}")
  elseif(RFLINK_TAG MATCHES "^firmware/v0[.]7[.]0-rc[.]([1-9][0-9]*)$")
    set(PROJECT_VER "0.7.0-rc.${CMAKE_MATCH_1}-n16r8-b${RF_BOARD_HASH}")
  elseif(RFLINK_TAG STREQUAL "firmware/v0.7.0")
    set(PROJECT_VER "0.7.0-n16r8-b${RF_BOARD_HASH}")
  else()
    message(FATAL_ERROR "Unsupported RFLINK_RELEASE_TAG; expected immutable firmware/v0.7.0 release")
  endif()
endif()
string(LENGTH "${PROJECT_VER}" RFLINK_VERSION_LENGTH)
if(RFLINK_VERSION_LENGTH GREATER 31)
  message(FATAL_ERROR "Firmware version exceeds ESP-IDF app_desc limit (31 bytes)")
endif()
project(rflink_universal)

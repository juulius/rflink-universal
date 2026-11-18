idf_component_register(SRCS "main.cpp" "rflink_web_ota.cpp" "rflink_mqtt.cpp" "rflink_mqtt_contract.cpp" "rflink_crash_report.cpp" "rflink_time.cpp" "rflink_status_led.cpp" INCLUDE_DIRS "." REQUIRES rflink_radio_hal rflink_decoder_core esp_wifi esp_event esp_netif esp_http_server esp_partition app_update nvs_flash esp_timer json espcoredump esp_driver_rmt)

# Generate compile-time flash strings from maintainable standalone UI files.
# The generated header lives ONLY in the build tree and does not change the
# versioned firmware source snapshot after it has been verified by CI.
set(RF_UI_INPUTS
  "${CMAKE_CURRENT_LIST_DIR}/webui/index.html"
  "${CMAKE_CURRENT_LIST_DIR}/webui/app.css"
  "${CMAKE_CURRENT_LIST_DIR}/webui/app.js"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${RF_UI_INPUTS})
file(READ "${CMAKE_CURRENT_LIST_DIR}/webui/index.html" RF_UI_HTML)
file(READ "${CMAKE_CURRENT_LIST_DIR}/webui/app.css" RF_UI_CSS)
file(READ "${CMAKE_CURRENT_LIST_DIR}/webui/app.js" RF_UI_JS)
configure_file("${CMAKE_CURRENT_LIST_DIR}/rflink_native_ui_assets.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/rflink_native_ui_assets.hpp" @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# Active 868-MHz transport is selected by the actual immutable release
# identity, not a UI label or a mutable runtime mode switch.
# Development builds from main (no release tag) and immutable Experimental
# firmware builds use the synchronous GDO DATA+CLOCK bench receiver.
# RC and Stable builds retain the former FIFO receiver until on-board
# synchronization, interrupt-load, RF-compatibility and TX restore are proven.
set(RFLINK_SYNC_868_EXPECTED OFF)
if(NOT DEFINED ENV{RFLINK_RELEASE_TAG} OR "$ENV{RFLINK_RELEASE_TAG}" STREQUAL "")
  set(RFLINK_SYNC_868_EXPECTED ON)
elseif("$ENV{RFLINK_RELEASE_TAG}" MATCHES "^firmware/v0[.]7[.]0-exp[.][1-9][0-9]*$")
  set(RFLINK_SYNC_868_EXPECTED ON)
elseif("$ENV{RFLINK_RELEASE_TAG}" MATCHES "^firmware/v0[.]7[.]0-rc[.][1-9][0-9]*$" OR
       "$ENV{RFLINK_RELEASE_TAG}" STREQUAL "firmware/v0.7.0")
  set(RFLINK_SYNC_868_EXPECTED OFF)
else()
  message(FATAL_ERROR "Unexpected RFLink release tag for synchronous 868 RX")
endif()

option(RFLINK_ENABLE_SYNC_868_BENCH
       "Experimental fixed 868 GDO DATA+CLOCK receiver instead of 63-byte FIFO"
       ${RFLINK_SYNC_868_EXPECTED})
# Refuse stale CMake caches: an Experimental-labelled image must not quietly
# contain the old FIFO receiver. Likewise RC/Stable must not inherit sync RX
# from an older Experimental build directory.
if(NOT RFLINK_ENABLE_SYNC_868_BENCH STREQUAL RFLINK_SYNC_868_EXPECTED)
  message(FATAL_ERROR
    "868 RX transport does not match release channel; use a clean build directory")
endif()
if(RFLINK_ENABLE_SYNC_868_BENCH)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE RFLINK_ENABLE_SYNC_868_BENCH=1)
  message(WARNING "Experimental 868 synchronous DATA+CLOCK RX ENABLED; hardware still unverified")
else()
  message(STATUS "868 MHz fixed FIFO RX selected for RC/Stable")
endif()


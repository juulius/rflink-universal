idf_component_register(SRCS "main.cpp" "rflink_web_ota.cpp" "rflink_mqtt.cpp" "rflink_mqtt_contract.cpp" "rflink_crash_report.cpp" INCLUDE_DIRS "." REQUIRES rflink_radio_hal rflink_decoder_core esp_wifi esp_event esp_netif esp_http_server esp_partition app_update nvs_flash esp_timer json espcoredump)

// Generate compile-time flash strings from maintainable standalone UI files.
// The generated header lives ONLY in the build tree and does not change the
// versioned firmware source snapshot after it has been verified by CI.
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

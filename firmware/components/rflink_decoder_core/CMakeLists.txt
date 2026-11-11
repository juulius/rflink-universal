idf_component_register(
 SRCS "src/rflink_native_decoder.cpp" "src/rflink_itho_codec.cpp" "src/rflink_ramses_codec.cpp" "src/rflink_ramses_tx.cpp" "src/rflink_user_vm.cpp" "src/rflink_user_library.cpp" "src/rflink_user_library_nvs.cpp" "src/rflink_user_rx_runtime.cpp"
 INCLUDE_DIRS "include"
 REQUIRES rflink_radio_hal nvs_flash)

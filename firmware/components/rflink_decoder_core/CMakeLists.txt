idf_component_register(
 SRCS "src/rflink_native_decoder.cpp" "src/rflink_itho_codec.cpp" "src/rflink_ramses_codec.cpp" "src/rflink_ramses_tx.cpp" "src/rflink_user_vm.cpp"
 INCLUDE_DIRS "include"
 REQUIRES rflink_radio_hal)

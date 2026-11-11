#pragma once
namespace esphome_test_stub { template<typename... T> inline void log(T&&...){ } }
#define ESP_LOGI(...) do{ ::esphome_test_stub::log(__VA_ARGS__); }while(0)
#define ESP_LOGW(...) do{ ::esphome_test_stub::log(__VA_ARGS__); }while(0)
#define ESP_LOGE(...) do{ ::esphome_test_stub::log(__VA_ARGS__); }while(0)
#define ESP_LOGD(...) do{ ::esphome_test_stub::log(__VA_ARGS__); }while(0)
#define ESP_LOGCONFIG(...) do{ ::esphome_test_stub::log(__VA_ARGS__); }while(0)

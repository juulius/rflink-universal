#include <atomic>
#include <cstdint>
#pragma once
void rflink_set_radio_snapshot(bool spi_ok, const bool detected[3]);
 // Publish the completed driver snapshot to HTTP handlers (release/acquire).
void rflink_publish_radio_providers();
// True only after normal startup or successful pending-OTA validation.
void rflink_publish_startup_complete();
bool rflink_start_web_ota();

namespace rflink {class PacketRx868;}
void rflink_set_rx_stats_provider(rflink::PacketRx868* receiver);

namespace rflink {class OokPulseReceiver;}
void rflink_set_ook_stats_providers(rflink::OokPulseReceiver* a, rflink::OokPulseReceiver* b);

namespace rflink {class NativeDecoder;}
void rflink_set_decoder_stats_provider(rflink::NativeDecoder* decoder);

namespace rflink {class RadioManager;}
void rflink_set_manager_provider(rflink::RadioManager* manager);

void rflink_set_rx_worker_cycles_provider(const std::atomic<uint32_t>* counter);

namespace rflink {class Cc1101SpiBus;}
void rflink_set_spi_stats_provider(const rflink::Cc1101SpiBus* bus);

namespace rflink {class LogStore;}
void rflink_set_log_store_provider(rflink::LogStore* logs);

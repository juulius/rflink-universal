# Features, implementation and test boundaries

| Feature | Source-level state | Limits |
| --- | --- | --- |
| Triple CC1101 radio setup | Present in default YAML | Module presence and hardware RF performance must be tested. |
| Radio monitor | Implemented | 32 recorded activity entries, not a continuous raw spectrum analyzer; TX command is not delivery confirmation. |
| Guided device learning | Implemented | Requires reliable captures and explicit approval. |
| Modular decoder packs | Implemented with examples | A listed decoder does not imply arbitrary devices of that brand work. |
| User decoder workflow | Capture, direct RX activation, bounded RX/TX interpreter, device profiles, JSON export and firmware promotion tool | Beginner TX is generated only for stable short captures; advanced recipes need protocol knowledge. It does not automatically synthesize arbitrary encryption or native C++. |
| Device persistence | Implemented with NVS snapshots | Hardware migration and failure recovery still need verification. |
| Home Assistant | ESPHome API plus optional MQTT Discovery | Broker connection and approval needed; MQTT is off by default. |
| Controlled TX | Built-in encoders and optional user TX recipes | Only approved device/actions on compatible radios; never interpret TX as acknowledged operation. User web TX has per-device opt-in; HA buttons also require MQTT and global config. |
| LED feedback | Implemented | Check actual wiring and LED type. |
| Web UI languages | Four selectable languages; separate NVS preference | Static and dynamic RFLink web UI copy is localized, including decoder onboarding overlays. Native ESPHome pages/logs remain separate. |
| Windows updater / packager | Existing PowerShell/BAT implementation | Full Windows execution and ESPHome compile still outstanding for this new source snapshot. |
| Web authentication | **Not implemented** | Trusted LAN only; do not expose directly to the internet. |
| PCB and enclosure | PCB designed by Juulius; enclosure in progress | No reviewed fabrication files or test results included in this software source package. |

## Important

Tests using software stubs and a successful compile are not substitutes for RF measurements, power integrity, antenna matching, EMI/EMC evaluation, regulatory compliance or field tests. Maintain a separate device-specific compatibility list when evidence exists.

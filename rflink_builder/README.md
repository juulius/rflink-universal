# RFLink Universal Builder (Home Assistant app)

Build native ESP-IDF firmware, compare GitHub changes, select published firmware releases and prepare safe USB/OTA testing on a separate board.

**Builder v0.6.0 (development preview)** — separate from the Radioactive v0.7 firmware. A native ESP-IDF firmware build has passed on GitHub Actions; installing this Builder app on Home Assistant OS and RF hardware operation have **not** been verified yet.

## What happens on first launch?

The Home Assistant app itself uses a relatively small Debian/Python image instead of the multi-gigabyte ESP-IDF Docker image. The web interface becomes available first. It then starts downloading and preparing the **official ESP-IDF v5.5.6 toolchain for ESP32-S3** in the background. You can watch the actual progress in **Build → Compiler setup → Live setup console**. Build actions remain disabled until setup passes a version check.

Compiler files live under the persistent Home Assistant `/data/toolchain` directory. They should not be downloaded again after an ordinary app restart. A failed download is visible and can be retried from the web interface. The large download and disk requirements still exist; they simply no longer block the app's initial web interface. We have not timed or validated the full Home Assistant setup in practice.

The default source repository is **`juulius/rflink-universal`**. It is selected automatically and does not need to be entered. A different repository can be configured under **Versions & GitHub → Use another repository (advanced)**.

The three-page interface is **Build**, **Versions & GitHub** and **Settings**. Details for firmware installation, MQTT credentials and board GPIOs remain available when you need them.

Follow the [Home Assistant guide](DOCS.md). Do not flash this prototype over an existing working v0.6.6 Radio Gaga device.

# Browser USB flashing from the user's PC (development, Builder v0.6.14)

**Implemented in Builder code; not yet tested with an actual ESP32-S3.**
A successful CI run proves package validation and client JavaScript syntax, not
that a Windows COM port is visible or that a gateway boots after flashing.

## What changed

Previously **USB** meant a serial port on the **Home Assistant host**.
With Builder v0.6.14 you can select **USB · This PC**: the ESP32 connects
directly to your Windows/macOS/Linux computer running the browser, using
Web Serial through the official secure [ESPHome Web](https://web.esphome.io/)
flashing tab. Home Assistant only **builds** and checks the firmware. It does
not need a locally attached USB cable.

The Builder sends the private first-install image directly from the
authenticated Home Assistant session to the browser. Where this page can
use Web Serial, it uses a locally bundled, pinned ESP Web Tools 10.4.0
installation dialog without opening a new tab. Only where HTTP/Ingress
permission policy blocks Web Serial does the popup offer an explicit
**Open official ESPHome Web installer** action using the existing
versioned **v1 postMessage flash handoff**. This browser-to-tab handoff is the same
approach ESPHome uses for dashboards without a secure Web Serial context.
It is **not** a background upload to a GitHub Release or ESPHome server.
However, the separate flasher tab executes code hosted by
`web.esphome.io` and **can read the embedded Wi-Fi/OTA credentials**. Use
only the official HTTPS site on a trusted PC. Never open an untrusted clone.

## First USB flash steps

1. **Before touching the board:** export a copy of the working v0.6.6
   ESPHome firmware/configuration and, ideally, take a **full flash backup**
   using the serial ROM bootloader. v0.7 uses a different 16 MB partition
   layout and the first install erases the previous contents.
2. Open Home Assistant → **RFLink Builder 0.6.13+**, create or select your
   configured **ESP32-S3 N16R8 test device**. Confirm GPIO/SPI mappings and
   choose the intended published v0.7 Experimental release, if available.
3. **Build firmware** with official ESP-IDF, and wait for the verified
   device-specific release manifest. Never flash another device's package.
4. Connect the ESP32-S3 using a **USB data cable to the computer running
   your browser**. Close any serial monitor that currently owns the COM port.
5. In RFLink Builder choose **Install firmware → USB · This PC**. Read the
   data-loss warning, type `TESTBOARD`, then choose
   **Flash using this computer**.
6. The Builder revalidates the device ID, immutable build job, GPIO profile,
   settings revision, 16 MB flash layout, all binary hashes and the unique
   NVS initialization image. It uses Espressif `esptool merge-bin` to
   produce one factory image containing the bootloader, partition table,
   private NVS, initial OTA partition data and application.
7. The **USB · This PC** popup stays in RFLink when Web Serial is permitted and opens its locally bundled ESP Web Tools installer. Use **Chrome or Edge**
   (or another browser with working Web Serial). Select the correct
   ESP32-S3 COM port and confirm installation. If needed, enter download
   mode using your board's **BOOT/EN** buttons. Flashing is not automatic.
8. After a reported successful write, wait for the ESP32 to boot. Check
   serial logs, Wi-Fi and `/api/status`, including release label and
   radio GPIO hash. **The Builder does not mark a device provisioned**
   until the physical RFLink is discovered/linked.

## Fallbacks

- **USB · HA host:** only if the ESP32 is plugged into the Home Assistant
  server/computer itself; this is the older method.
- **Download ZIP:** private package for a manual install on another computer.
  Contains NVS with unencrypted Wi-Fi/OTA credentials; never publish it.
- **OTA:** only after an actual native RFLink device is online, linked and
  compatible; changing from ESPHome v0.6.6 to native v0.7 via OTA is **not
  supported**. Reverting to v0.6.6 also needs a deliberate USB reflash.

## Why we use a separate HTTPS flasher tab

Browser access to COM ports needs a secure context and a user gesture.
Home Assistant Ingress can be embedded in an iframe with permissions that
prevent direct Web Serial access. The external ESPHome Web page has its own
HTTPS secure origin. The Builder opens it only after the user clicks the fallback button,
and transfers bytes exclusively to its known origin using a random
nonce and verified message source. No permissive CORS rules are needed.
The in-app flasher files are self-hosted, with no remote JavaScript
loader; CSP permits local assets and blob firmware fetches only.

## Current limitations

- No live Windows USB test yet; browser permissions, the Espressif merge
  command in the Home Assistant toolchain, USB CDC/USB-UART selection, full
  erase, board boot/PSRAM, Wi-Fi NVS, and restoration to v0.6.6 **need to be
  tested on hardware**.
- Web Serial identifies the chip family, not the physical RFLink board's
  CC1101 wiring. You must select the correct N16R8 print and confirm the
  GPIO map in Builder. There is no way to guarantee that a different
  ESP32-S3 is not mistakenly selected in the browser.
- Wi-Fi and OTA credentials remain plaintext in the private factory image
  during flashing, because the existing native v0.7 NVS provisioning
  requires plaintext. Do not share the image or a browser diagnostic dump.
- The user must authorize USB access and confirm erase/write in the new
  tab. Builder does not automate USB device selection or claim physical
  success based solely on browser progress.

# A small, bootable Home Assistant app. The official ESP-IDF toolchain is
# installed AFTER the web UI starts, with progress shown inside the Builder.
# Everything expensive is stored under the persistent /data mount.
FROM python:3.12-slim-bookworm

ARG BUILD_VERSION=0.6.41
ARG BUILD_ARCH=amd64
LABEL io.hass.version="${BUILD_VERSION}" \
      io.hass.type="app" \
      io.hass.arch="${BUILD_ARCH}"

# Prevent one glibc malloc arena per high-concurrency scan worker. This is
# configured before Python starts; it affects native heap retention, not
# the ESP-IDF compiler's result or firmware contents.
ENV MALLOC_ARENA_MAX=2

ENV RFLINK_BUILDER_VERSION="${BUILD_VERSION}" \
    PYTHONUNBUFFERED=1 \
    PIP_DISABLE_PIP_VERSION_CHECK=1

# Official ESP-IDF Linux prerequisites, but no ESP-IDF checkout or compiler.
# Install only from Debian's signed repositories. The ESP32-S3 compiler and
# its Python environment are downloaded later by toolchain.py.
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash ca-certificates git cmake ninja-build flex bison gperf \
    build-essential pkg-config ccache libffi-dev libssl-dev \
    libusb-1.0-0 libudev1 dfu-util wget curl xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY *.py /app/
COPY static/ /app/static/
# Bundle the official Espressif browser flasher at image build time. At
# runtime there is no CDN access and the strict same-origin CSP remains.
# Pinned v0.7.1 is Apache-2.0; LICENSE is retained beside the bundle.
RUN mkdir -p /app/static/vendor/esptool-js \
    && curl --fail --location --silent --show-error --retry 3 \
       --connect-timeout 15 --max-time 120 \
       'https://unpkg.com/esptool-js@0.7.1/bundle.js' \
       --output /app/static/vendor/esptool-js/bundle.js \
    && test -s /app/static/vendor/esptool-js/bundle.js \
    && echo 'd6a4b0b7c9b747a7370cfa976164a962f56332d5faa7c3b45cdb95a2a8b1d456  /app/static/vendor/esptool-js/bundle.js' | sha256sum -c -
COPY default_source.zip /app/default_source.zip
# Fully offline 433/868 replay: pin the same native RAMSES-II/Itho C++ codecs
# from the bundled firmware source. Compile once in the Builder image, not on
# every upload. Never accept compilers, paths or user code from uploaded RF logs.
COPY replay_tools/ /app/replay_tools/
RUN python3 -c 'from pathlib import Path; from zipfile import ZipFile; z=ZipFile("/app/default_source.zip"); p=("firmware/components/rflink_decoder_core/include/rflink_ramses_codec.hpp","firmware/components/rflink_decoder_core/include/rflink_itho_codec.hpp","firmware/components/rflink_decoder_core/src/rflink_ramses_codec.cpp","firmware/components/rflink_decoder_core/src/rflink_itho_codec.cpp"); [( (Path("/app")/n).parent.mkdir(parents=True,exist_ok=True), (Path("/app")/n).write_bytes(z.read(n))) for n in p]' \
    && g++ -std=c++17 -O2 -I /app/firmware/components/rflink_decoder_core/include \
       /app/replay_tools/rf_replay_codecs.cpp \
       /app/firmware/components/rflink_decoder_core/src/rflink_ramses_codec.cpp \
       /app/firmware/components/rflink_decoder_core/src/rflink_itho_codec.cpp \
       -o /app/replay_tools/rf_replay_codecs \
    && chmod 755 /app/replay_tools/rf_replay_codecs
ENV RFLINK_RF_REPLAY_CODECS_BIN=/app/replay_tools/rf_replay_codecs

CMD ["python3", "-u", "/app/server.py"]

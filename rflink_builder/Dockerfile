# A small, bootable Home Assistant app. The official ESP-IDF toolchain is
# installed AFTER the web UI starts, with progress shown inside the Builder.
# Everything expensive is stored under the persistent /data mount.
FROM python:3.12-slim-bookworm

ARG BUILD_VERSION=0.6.8
ARG BUILD_ARCH=amd64
LABEL io.hass.version="${BUILD_VERSION}" \
      io.hass.type="app" \
      io.hass.arch="${BUILD_ARCH}"

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
COPY default_source.zip /app/default_source.zip

CMD ["python3", "-u", "/app/server.py"]

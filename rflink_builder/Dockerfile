# Official pre-provisioned ESP-IDF toolchain, for amd64 and arm64.
# The toolchain runs on the Home Assistant machine, not on the user's PC.
FROM espressif/idf:v5.5.6

ARG BUILD_VERSION=0.5.1
ARG BUILD_ARCH=amd64
LABEL io.hass.version="${BUILD_VERSION}" \
      io.hass.type="app" \
      io.hass.arch="${BUILD_ARCH}"

WORKDIR /app
COPY *.py /app/
COPY static/ /app/static/
COPY default_source.zip /app/default_source.zip

# Keep the image's official entrypoint; it prepares PATH, IDF tools and Python env.
CMD ["python3", "-u", "/app/server.py"]

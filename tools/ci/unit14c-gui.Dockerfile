FROM qtmaps-v1c-runtime:latest

# Local-only GUI proof for the exact internal archive. Build before disabling
# networking; the candidate is mounted read-only when this image is run.
RUN apt-get update && apt-get install -y --no-install-recommends \
    xvfb xauth imagemagick \
    && rm -rf /var/lib/apt/lists/*

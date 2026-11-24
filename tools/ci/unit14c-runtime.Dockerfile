FROM ubuntu:26.04

# Match the observed sufficient package set from the 14A clean Desktop VM.
# The image is built online, then run with --network none for the smoke.
RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt5location5 libqt5location5-plugins libqt5positioning5 \
    libqt5sql5-sqlite qml-module-qtlocation qml-module-qtpositioning \
    qml-module-qtquick2 qml-module-qtquick-window2 \
    libgl1-mesa-dri libgl1 fonts-noto-core python3 iproute2 coreutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /proof

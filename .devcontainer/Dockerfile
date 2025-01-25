FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

# Remove Ubuntu from image since it is preventing the vscode user from claiming the UID
#RUN userdel --remove ubuntu

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends \
        gdb-multiarch \
        python3-venv \
        libusb-1.0-0 \
        libhidapi-hidraw0 \
        libhidapi-libusb0 \
        libftdi1-2 \
        usbutils \
        udev \
        psmisc \
        lsof \
        less \
        curl \
        picocom


RUN <<EOF
usermod -a -G dialout vscode
usermod -a -G plugdev vscode
usermod -a -G root vscode  
EOF
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu24.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

COPY .devcontainer/reinstall-cmake.sh /tmp/
RUN <<EOF
if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then
	chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE};
fi
rm -f /tmp/reinstall-cmake.sh
EOF

# Update system and set noninteractive env
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update

# Required packages for building gcc & binutils
RUN apt-get -y install --no-install-recommends \
	sudo \
	build-essential \
	bison \
	flex \
	libgmp-dev \
	libmpfr-dev \
	texinfo \
	file \
	python3-dev

# Required packages for building qemu
RUN apt-get -y install --no-install-recommends \
	git \
	libglib2.0-dev \
	libfdt-dev \
	libpixman-1-dev \
	zlib1g-dev \
	ninja-build \
	libaio-dev \
	libbluetooth-dev \
	libcapstone-dev \
	libbrlapi-dev \
	libbz2-dev \
	libcap-ng-dev \
	libcurl4-gnutls-dev \
	libgtk-3-dev \
	libibverbs-dev \
	libjpeg8-dev \
	libncurses5-dev \
	libnuma-dev \
	librbd-dev \
	librdmacm-dev \
	libsasl2-dev \
	libsdl2-dev \
	libseccomp-dev \
	libsnappy-dev \
	libssh-dev \
	libvde-dev \
	libvdeplug-dev \
	libvte-2.91-dev \
	libxen-dev \
	liblzo2-dev \
	valgrind \
	xfslibs-dev \
	libnfs-dev \
	libiscsi-dev \
	python3-venv \
	libslirp-dev \
	ovmf

# Required packages for building test apps in userspace
RUN apt-get -y install --no-install-recommends \
	mingw-w64 \
	libtool \
	libltdl-dev

# Required packages for building the OS and misc
RUN apt-get -y install --no-install-recommends \
	grub2-common \
	xorriso \
	mtools \
	grub-common \
	grub-efi-amd64-bin \
	grub-efi-amd64-signed \
	grub-gfxpayload-lists \
	grub-pc-bin \
	grub-pc \
	grub2-common \
	pip \
	cmake

# Required packages for vscode
RUN apt-get -y install --no-install-recommends \
	ripgrep

# Install git-cliff
RUN pip install git-cliff --break-system-packages

# Install meson
RUN pip install meson --break-system-packages

# Install other utilities
RUN apt-get -y install --no-install-recommends \
	npm

# Configure git
RUN <<EOF
git config --global advice.detachedHead false
git config --global core.autocrlf input
git config --global init.defaultbranch master
EOF

# Set display environment variable for QEMU
ENV DISPLAY=:0
ENV NO_AT_BRIDGE=1

# Set PATH
RUN <<EOF
echo PATH=$PATH:/workspaces/Fennix/cross/bin >> /etc/profile
EOF

ENV CHMOD_KVM=1

# Remove autoconf & automake
RUN sudo apt-get -y remove autoconf automake

# Required packages for building gcc & binutils
RUN <<EOF
wget https://launchpad.net/ubuntu/+archive/primary/+files/autoconf_2.69-11.1_all.deb -O /tmp/autoconf.deb
sudo dpkg --force-all -i /tmp/autoconf.deb
EOF

# Required packages for building gcc & binutils
RUN <<EOF
wget https://ftp.gnu.org/gnu/automake/automake-1.15.1.tar.gz -O /tmp/automake.tar.gz
tar -xzf /tmp/automake.tar.gz -C /tmp
cd /tmp/automake-1.15.1
./configure && make && sudo make install
EOF

COPY tools/Makefile tools/ReadEthernetPackets.c tools/ErrorParser.cpp tools/binutils-gdb.patch tools/gcc.patch /tmp/fennix-tools/
RUN make -C /tmp/fennix-tools all && \
	make -C /tmp/fennix-tools clean && \
	rm -rf /tmp/fennix-tools

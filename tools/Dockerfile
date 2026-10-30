FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/usr/local/bin:${PATH}"

ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH="/usr/local/cargo/bin:${PATH}"

RUN apt-get update

# QoL packages
RUN apt-get -y install --no-install-recommends \
	sudo \
	wget \
	curl \
	git \
	nano \
	gnupg \
	ca-certificates \
	lsb-release \
	apt-utils \
	bash-completion

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal

RUN rustup target add x86_64-unknown-none aarch64-unknown-none

# Required packages for building gcc & binutils
RUN apt-get -y install --no-install-recommends \
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

RUN rm -rf /var/lib/apt/lists/*

RUN wget https://launchpad.net/ubuntu/+archive/primary/+files/autoconf_2.69-11.1_all.deb -O /tmp/autoconf.deb && \
	dpkg --force-all -i /tmp/autoconf.deb && \
	wget https://ftp.gnu.org/gnu/automake/automake-1.15.1.tar.gz -O /tmp/automake.tar.gz && \
	tar -xzf /tmp/automake.tar.gz -C /tmp && \
	cd /tmp/automake-1.15.1 && \
	./configure && make -j"$(nproc)" && make install && \
	rm -rf /tmp/autoconf.deb /tmp/automake.tar.gz /tmp/automake-1.15.1

COPY tools/Makefile tools/ReadEthernetPackets.c tools/ErrorParser.cpp tools/binutils-gdb.patch tools/gcc.patch tools/qemu.patch /tmp/fennix-tools/
RUN make -C /tmp/fennix-tools all && \
	make -C /tmp/fennix-tools clean && \
	rm -rf /tmp/fennix-tools

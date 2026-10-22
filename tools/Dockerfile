FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/usr/local/bin:${PATH}"

RUN apt-get update && apt-get install -y --no-install-recommends \
	sudo \
	build-essential \
	bison \
	flex \
	libgmp-dev \
	libmpfr-dev \
	libmpc-dev \
	texinfo \
	file \
	python3-dev \
	git \
	wget \
	ca-certificates \
	perl \
	m4 \
	libglib2.0-dev \
	libfdt-dev \
	libpixman-1-dev \
	zlib1g-dev \
	ninja-build \
	meson \
	pkg-config \
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
	ovmf \
	&& rm -rf /var/lib/apt/lists/*

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

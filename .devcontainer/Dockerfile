FROM ghcr.io/enderice2/fennix-toolchain:latest

ARG USERNAME=vscode

RUN usermod --login "$USERNAME" --home "/home/$USERNAME" --move-home ubuntu && \
	groupmod --new-name "$USERNAME" ubuntu && \
	printf '%s ALL=(root) NOPASSWD:ALL\n' "$USERNAME" > "/etc/sudoers.d/$USERNAME" && \
	chmod 0440 "/etc/sudoers.d/$USERNAME"

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

COPY reinstall-cmake.sh /tmp/
RUN <<EOF
if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then
	chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE};
fi
rm -f /tmp/reinstall-cmake.sh
EOF

# Update system and set noninteractive env
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update

# Fix autoconf dependencies...
RUN sudo apt -y --fix-broken install

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
	python3-pip \
	cmake

# Required packages for vscode
RUN apt-get -y install --no-install-recommends \
	ripgrep

# Install git-cliff
RUN python3 -m pip install git-cliff --break-system-packages

# Install meson
RUN python3 -m pip install meson --break-system-packages

# Install other utilities
RUN apt-get -y install --no-install-recommends \
	npm

# Configure git
RUN <<EOF
git config --system advice.detachedHead false
git config --system core.autocrlf input
git config --system init.defaultbranch master
EOF

# Set display environment variable for QEMU
ENV DISPLAY=:0
ENV NO_AT_BRIDGE=1

# Set PATH
RUN <<EOF
echo PATH=$PATH:/usr/local/bin >> /etc/profile
EOF

ENV USING_DEVCONTAINER=1

ENV PATH="/usr/local/bin:${PATH}"

USER $USERNAME

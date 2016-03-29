DESCRIPTION = "Small image capable of booting a device with support for the \
Minimal MTD Utilities, which let the user interact with the MTD subsystem in \
the kernel to perform operations on flash devices."

SUMMARY = "A small image just capable of allowing a device to boot."

IMAGE_INSTALL = "packagegroup-core-boot ${ROOTFS_PKGMANAGE_BOOTSTRAP} ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

IMAGE_INSTALL += "olt-app"

LICENSE = "MIT"

inherit core-image


IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE_append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "" ,d)}"


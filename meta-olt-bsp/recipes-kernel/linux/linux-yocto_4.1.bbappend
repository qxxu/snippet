DESCRIPTION="OLT kernel"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

PR := "${PR}.1"

COMPATIBLE_MACHINE = "olt"
COMPATIBLE_MACHINE_olt = "olt"
#COMPATIBLE_MACHINE_append = "|olt"

#SRC_URI += "file://defconfig \ 
#            file://olt.cfg"

#SRC_URI += "\ 
#            file://olt.cfg"

KBUILD_DEFCONFIG = "vexpress_defconfig"

# replace these SRCREVs with the real commit ids once you've had
# the appropriate changes committed to the upstream linux-yocto repo
#SRCREV_machine_pn-linux-yocto_olt-bsp ?= "${AUTOREV}"
#SRCREV_meta_pn-linux-yocto_olt-bsp ?= "${AUTOREV}"
#LINUX_VERSION = "4.1"

DESCRIPTION="OLT kernel"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

PR := "${PR}.1"

COMPATIBLE_MACHINE = "olt"
COMPATIBLE_MACHINE_olt = "olt"
#COMPATIBLE_MACHINE_append = "|olt"

SRC_URI += "\ 
            file://mmu.cfg"

KBUILD_DEFCONFIG = "vexpress_defconfig"

SRCREV_machine_olt ?= "857048f10bfe7089ca6007e72431f1c098b07115"
COMPATIBLE_MACHINE_olt = "olt"
KBRANCH_olt = "standard/arm-versatile-926ejs"

SUMMARY = "DHCP Ipmonitor application"
SECTION = "examples"

LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

SRC_URI:append = "file://CMakeLists.txt "
SRC_URI:append = "file://src/ "
#SRC_URI:append = "file://systemd/ "
SRC_URI:append = "file://systemd/Ipmonitor.service "

DEPENDS = "boost nlohmann-json sdbusplus"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_AUTO_ENABLE = "enable"
SYSTEMD_SERVICE:${PN} = "Ipmonitor.service"

FILES:${PN}:append = " {systemd_unitdir}/system/Ipmonitor.service"

do_install(){
         install -d ${D}${bindir}
         install -m 0755 ipmon ${D}${bindir}

         install -d ${D}${systemd_unitdir}/system/
         install -m 0644 ${WORKDIR}/systemd/Ipmonitor.service ${D}${systemd_unitdir}/system/
}

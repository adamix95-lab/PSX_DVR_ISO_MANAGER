cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(desr_network_probe C)

set(SOURCES src/main.c src/probe_config.c)

# Match the verified v0.5.3 DVR + USB startup. No HDD write modules.
set(USB_IRX iomanX fileXio bdm bdmfs_fatfs usbd_mini usbmass_bd_mini)
set(DVR_IRX dev9_ns dvrdrv dvrfile)
# PS2SDK native EE-side TCP/IP stack with netman/smap, NOT ps2ip-nm.
set(NET_IRX netman smap)

foreach(name ${USB_IRX} ${NET_IRX})
  set(path "$ENV{PS2SDK}/iop/irx/${name}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${name}_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${path}" "${generated}" "${name}_irx"
    DEPENDS "${path}" VERBATIM)
  list(APPEND SOURCES "${generated}")
endforeach()
foreach(name ${DVR_IRX})
  set(path "${CMAKE_SOURCE_DIR}/../dvr-iso-manager/modules/${name}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${name}_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${path}" "${generated}" "${name}_irx"
    DEPENDS "${path}" VERBATIM)
  list(APPEND SOURCES "${generated}")
endforeach()

add_executable(DVR_NET_PROBE.ELF ${SOURCES})
target_include_directories(DVR_NET_PROBE.ELF PRIVATE include)
target_link_libraries(DVR_NET_PROBE.ELF patches fileXio cdvd ps2ip netman libdebug.a m)
target_compile_options(DVR_NET_PROBE.ELF PRIVATE -Os -Wall -Wextra)

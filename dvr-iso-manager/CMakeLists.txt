cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(desr_dvr_iso_manager C)

set(SOURCES
  src/main.c src/iso_utils.c src/hdd_boot_config.c src/cover_index.c src/cover_runtime.c
  src/cover_preview.c src/cover_color.c src/cover_report.c src/lan_art_sync.c
  src/net/probe_config.c src/net/probe_png.c
  src/net/art_sync.c src/net/art_sync_runtime.c)
# Drivers from PS2SDK, plus DVR drivers from the user's confirmed PR #228 build.
set(PS2SDK_IRX iomanX fileXio sio2man mcman mcserv freepad bdm bdmfs_fatfs usbd_mini usbmass_bd_mini netman smap)
set(DVR_IRX dev9_ns dvrdrv dvrfile)

foreach(name ${PS2SDK_IRX})
  set(path "$ENV{PS2SDK}/iop/irx/${name}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${name}_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${path}" "${generated}" "${name}_irx"
    DEPENDS "${path}" VERBATIM)
  list(APPEND SOURCES "${generated}")
endforeach()
foreach(name ${DVR_IRX})
  set(path "${CMAKE_CURRENT_SOURCE_DIR}/modules/${name}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${name}_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${path}" "${generated}" "${name}_irx"
    DEPENDS "${path}" VERBATIM)
  list(APPEND SOURCES "${generated}")
endforeach()

# Keep USB embedded modules unchanged; use separate APA/PFS sources for HDD.
set(HDD_PFS_SOURCES)
foreach(name ps2dev9 ps2atad ps2hdd ps2fs)
  set(path "$ENV{PS2SDK}/iop/irx/${name}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${name}_hdd_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${path}" "${generated}" "${name}_irx"
    DEPENDS "${path}" VERBATIM)
  list(APPEND HDD_PFS_SOURCES "${generated}")
endforeach()

add_executable(DVR_ISO_MANAGER.ELF ${SOURCES})
target_include_directories(DVR_ISO_MANAGER.ELF PRIVATE include "$ENV{PS2SDK}/ports/include")
target_link_libraries(DVR_ISO_MANAGER.ELF pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
target_compile_options(DVR_ISO_MANAGER.ELF PRIVATE -Os -Wall -Wextra)

# Staged 1.0 HDD UI using the same USB menu and cover renderer.
# READ-ONLY PREVIEW: no CNF, ART or INDEX writes; X launch disabled.
# First verify PP.DVRISO PFS init/access on real hardware.
add_executable(DVR_ISO_MANAGER_HDD_PREVIEW.ELF EXCLUDE_FROM_ALL ${SOURCES} ${HDD_PFS_SOURCES})
target_compile_definitions(DVR_ISO_MANAGER_HDD_PREVIEW.ELF PRIVATE DESR_HDD_PREVIEW=1)
target_include_directories(DVR_ISO_MANAGER_HDD_PREVIEW.ELF PRIVATE include "$ENV{PS2SDK}/ports/include")
target_link_libraries(DVR_ISO_MANAGER_HDD_PREVIEW.ELF pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
target_compile_options(DVR_ISO_MANAGER_HDD_PREVIEW.ELF PRIVATE -Os -Wall -Wextra)

# STANDALONE, READ-ONLY HDD LAN diagnostic.  It deliberately inherits the
# PREVIEW compile gate so all launch/write commands remain disabled.
add_executable(DVR_ISO_MANAGER_HDD_LAN_PROBE.ELF EXCLUDE_FROM_ALL ${SOURCES} ${HDD_PFS_SOURCES})
target_compile_definitions(DVR_ISO_MANAGER_HDD_LAN_PROBE.ELF PRIVATE DESR_HDD_PREVIEW=1 DESR_HDD_ART_PROBE=1)
target_include_directories(DVR_ISO_MANAGER_HDD_LAN_PROBE.ELF PRIVATE include "$ENV{PS2SDK}/ports/include")
target_link_libraries(DVR_ISO_MANAGER_HDD_LAN_PROBE.ELF pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
target_compile_options(DVR_ISO_MANAGER_HDD_LAN_PROBE.ELF PRIVATE -Os -Wall -Wextra)

# Interactive HDD builds require the same patched SDK as the tested NETFIX.
# USB, the read-only preview and the diagnostic can use the stock SDK.
option(DESR_BUILD_HDD_NETFIX "Build the interactive HDD Manager with the patched SDK" OFF)
if(DESR_BUILD_HDD_NETFIX)
  if(NOT EXISTS "$ENV{PS2SDK}/HDD_NETFIX_BUILD.txt")
    message(FATAL_ERROR "HDD Manager requires its patched SDK; run hdd-netfix/build_sdk.sh first")
  endif()
  add_executable(DVR_ISO_MANAGER_HDD_TEST.ELF EXCLUDE_FROM_ALL ${SOURCES} ${HDD_PFS_SOURCES})
  target_compile_definitions(DVR_ISO_MANAGER_HDD_TEST.ELF PRIVATE DESR_HDD_TEST=1 DESR_HDD_NETFIX=1)
  target_include_directories(DVR_ISO_MANAGER_HDD_TEST.ELF PRIVATE include "$ENV{PS2SDK}/ports/include")
  target_link_libraries(DVR_ISO_MANAGER_HDD_TEST.ELF pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
  target_compile_options(DVR_ISO_MANAGER_HDD_TEST.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(DVR_ISO_MANAGER_HDD_TEST.ELF PRIVATE "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/HDD_NETFIX.map")
endif()

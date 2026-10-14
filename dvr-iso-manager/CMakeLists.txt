cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(desr_dvr_iso_manager C)

set(SOURCES src/main.c src/iso_utils.c)
# Drivers from PS2SDK, plus DVR drivers from the user's confirmed PR #228 build.
set(PS2SDK_IRX iomanX fileXio sio2man mcman mcserv freepad bdm bdmfs_fatfs usbd_mini usbmass_bd_mini)
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

add_executable(DVR_ISO_MANAGER.ELF ${SOURCES})
target_include_directories(DVR_ISO_MANAGER.ELF PRIVATE include)
target_link_libraries(DVR_ISO_MANAGER.ELF pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(DVR_ISO_MANAGER.ELF PRIVATE -Os -Wall -Wextra)

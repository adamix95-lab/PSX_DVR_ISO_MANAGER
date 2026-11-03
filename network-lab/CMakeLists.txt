cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(desr_hdd_network_lab C)

# One identical HTTP/PNG implementation. The only intended difference among
# A/B/C is driver composition; A and B never load ATA or mount PFS.
set(LAB_SOURCES src/nettest.c ../dvr-iso-manager/src/net/probe_png.c)
set(LAB_COMMON_IRX iomanX fileXio sio2man mcman mcserv freepad netman smap)

function(embed_driver target name location)
  set(out "${CMAKE_CURRENT_BINARY_DIR}/${target}_${name}_irx.c")
  add_custom_command(OUTPUT "${out}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${location}" "${out}" "${name}_irx"
    DEPENDS "${location}" VERBATIM)
  target_sources(${target} PRIVATE "${out}")
endfunction()

foreach(mode A B C CLEAN)
  add_executable(DVR_NETTEST_${mode}.ELF ${LAB_SOURCES})
  target_compile_definitions(DVR_NETTEST_${mode}.ELF PRIVATE LAB_MODE_${mode}=1)
  target_include_directories(DVR_NETTEST_${mode}.ELF PRIVATE
    src ../dvr-iso-manager/include "$ENV{PS2SDK}/ports/include")
  target_compile_options(DVR_NETTEST_${mode}.ELF PRIVATE -Os -Wall -Wextra)
  target_link_libraries(DVR_NETTEST_${mode}.ELF
    pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a m)
  foreach(name ${LAB_COMMON_IRX})
    embed_driver(DVR_NETTEST_${mode}.ELF ${name} "$ENV{PS2SDK}/iop/irx/${name}.irx")
  endforeach()
  if(mode STREQUAL "A")
    embed_driver(DVR_NETTEST_A.ELF dev9_ns
      "${CMAKE_CURRENT_SOURCE_DIR}/../dvr-iso-manager/modules/dev9_ns.irx")
  else()
    embed_driver(DVR_NETTEST_${mode}.ELF ps2dev9 "$ENV{PS2SDK}/iop/irx/ps2dev9.irx")
  endif()
  if(mode STREQUAL "C" OR mode STREQUAL "CLEAN")
    foreach(name ps2atad ps2hdd ps2fs)
      embed_driver(DVR_NETTEST_${mode}.ELF ${name} "$ENV{PS2SDK}/iop/irx/${name}.irx")
    endforeach()
    embed_driver(DVR_NETTEST_${mode}.ELF dvrdrv
      "${CMAKE_CURRENT_SOURCE_DIR}/../dvr-iso-manager/modules/dvrdrv.irx")
    embed_driver(DVR_NETTEST_${mode}.ELF dvrfile
      "${CMAKE_CURRENT_SOURCE_DIR}/../dvr-iso-manager/modules/dvrfile.irx")
  endif()
endforeach()

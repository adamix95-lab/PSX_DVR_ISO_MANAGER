cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(psx_dvr_hdd_diagnostic C)

# Stage A attaches to wLaunchELF R3Z's already initialized IOP stack.
# No IRX is embedded or loaded, no HDD operations can write data.
add_executable(PSX_DVR_HDD_DIAG_A.ELF src/main.c)
target_link_libraries(PSX_DVR_HDD_DIAG_A.ELF fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_A.ELF PRIVATE -Os -Wall -Wextra -Werror)

# Independent diagnostic binaries. HELLO tests only rendering; DIAG B
# prints stage markers before each blocking RPC call. Neither writes to HDD.
add_executable(PSX_DVR_HDD_HELLO.ELF src/hello.c)
target_link_libraries(PSX_DVR_HDD_HELLO.ELF libdebug.a)
target_compile_options(PSX_DVR_HDD_HELLO.ELF PRIVATE -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_B.ELF src/step_diag.c)
target_link_libraries(PSX_DVR_HDD_DIAG_B.ELF fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_B.ELF PRIVATE -Os -Wall -Wextra -Werror)

# Physical DESR-7000 DIAG B froze before fileXioInit returned. DIAG C follows
# v0.8's self-contained IOP initialization order; it intentionally does NOT
# access HDD or attempt a PFS mount. Embed only the two required IRX modules.
set(FILEXIO_DIAG_EMBEDDED)
foreach(module iomanX fileXio)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND FILEXIO_DIAG_EMBEDDED "${source}")
endforeach()

add_executable(PSX_DVR_HDD_DIAG_C.ELF
  src/filexio_reset_diag.c
  ${FILEXIO_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_C.ELF
  patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_C.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

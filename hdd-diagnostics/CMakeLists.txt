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

# HDD DIAG D: rebuild the PS2 APA/PFS stack in the same order as RepairBox
# after the successful hardware test C. No DVR or filesystem-write modules.
set(APA_DIAG_EMBEDDED)
foreach(module iomanX fileXio ps2dev9 ps2atad ps2hdd ps2fs)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_apa_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND APA_DIAG_EMBEDDED "${source}")
endforeach()

add_executable(PSX_DVR_HDD_DIAG_D.ELF
  src/apa_pfs_diag.c
  ${APA_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_D.ELF
  patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_D.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG E — physical D passed: keep PS2 APA PFS1 mounted READ ONLY while
# initializing the PSX DVR driver stack. Build a separate, opt-in test ELF.
# PS2SDK provides the PS2 HDD IRX; DVR IRX are the same files as v0.8.
set(COEXIST_DIAG_EMBEDDED)
foreach(module iomanX fileXio ps2dev9 ps2atad ps2hdd ps2fs)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_coexist_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND COEXIST_DIAG_EMBEDDED "${source}")
endforeach()
foreach(module dvrdrv dvrfile)
  set(irx "${CMAKE_CURRENT_SOURCE_DIR}/../dvr-iso-manager/modules/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_coexist_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND COEXIST_DIAG_EMBEDDED "${source}")
endforeach()

add_executable(PSX_DVR_HDD_DIAG_E.ELF
  src/apa_dvr_coexist_diag.c
  ${COEXIST_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_E.ELF
  patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_E.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG F: isolated controller START -> normal ELF exit (OSD/XMB candidate).
# This test purposely has NO PS2 HDD, DVR or PFS access whatsoever.
set(EXIT_DIAG_EMBEDDED)
foreach(module iomanX fileXio sio2man mcman mcserv freepad)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_exit_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND EXIT_DIAG_EMBEDDED "${source}")
endforeach()

add_executable(PSX_DVR_HDD_DIAG_F.ELF
  src/pad_exit_diag.c
  ${EXIT_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_F.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_F.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG G — reuse proven APA/DVR IRX and add F's gamepad stack.
set(G_DIAG_EMBEDDED ${COEXIST_DIAG_EMBEDDED})
foreach(module sio2man mcman mcserv freepad)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${module}_g_diag_irx.c")
  add_custom_command(
    OUTPUT "${source}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${source}" "${module}_irx"
    DEPENDS "${irx}"
    VERBATIM)
  list(APPEND G_DIAG_EMBEDDED "${source}")
endforeach()
add_executable(PSX_DVR_HDD_DIAG_G.ELF
  src/apa_dvr_pad_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_G.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_G.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG H — same proven G driver stack; only reads manually staged boot files.
# Keep E/F/G binaries and sources unchanged.
add_executable(PSX_DVR_HDD_DIAG_H.ELF
  src/apa_dvr_staged_files_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_H.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_H.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG I — no game launch: read-only BOOT.CNF and HDD path audit.
add_executable(PSX_DVR_HDD_DIAG_I.ELF
  src/apa_dvr_boot_path_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_I.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_I.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG J — first manual, explicit X-triggered HDD OSDMenu chainload experiment.
# Same G/H/I read-only driver stack; no changes to previous diagnostic sources.
add_executable(PSX_DVR_HDD_DIAG_J.ELF
  src/apa_dvr_hdd_launch_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_J.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_J.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG K: raw HDD ELF execution control; no OSDMenu / Neutrino.
add_executable(PSX_DVR_HDD_DIAG_K.ELF
  src/apa_dvr_raw_elf_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_K.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_K.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG L: two isolated ELFs. Launcher runs via UDPFS, target executes from HDD.
# Target reinitializes the known-good IOP / APA / DVR stack AFTER chainload.
add_executable(PSX_DVR_HDD_DIAG_L.ELF
  src/apa_dvr_postboot_diag.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_L.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_L.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_L_LAUNCH.ELF
  src/apa_dvr_postboot_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_L_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_L_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG M: explicit OSDMenu APA module order, including ps2hdd-osd and secrsif.
# M launcher reuses G/L's known-good IOP init and stays external via UDPFS.
set(M_OSD_EMBEDDED)
foreach(module iomanX fileXio sio2man mcman mcserv freepad ps2dev9 bdm bdmfs_fatfs ata_bd ps2fs secrsif)
  set(irx "$ENV{PS2SDK}/iop/irx/${module}.irx")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/${module}_m_osd_irx.c")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "$ENV{PS2SDK}/bin/bin2c" "${irx}" "${generated}" "${module}_irx"
    DEPENDS "${irx}" VERBATIM)
  list(APPEND M_OSD_EMBEDDED "${generated}")
endforeach()
set(m_osd_hdd_irx "$ENV{PS2SDK}/iop/irx/ps2hdd-osd.irx")
set(m_osd_hdd_gen "${CMAKE_CURRENT_BINARY_DIR}/ps2hdd_osd_m_osd_irx.c")
add_custom_command(OUTPUT "${m_osd_hdd_gen}"
  COMMAND "$ENV{PS2SDK}/bin/bin2c" "${m_osd_hdd_irx}" "${m_osd_hdd_gen}" "ps2hdd_osd_irx"
  DEPENDS "${m_osd_hdd_irx}" VERBATIM)
list(APPEND M_OSD_EMBEDDED "${m_osd_hdd_gen}")

add_executable(PSX_DVR_HDD_DIAG_M.ELF
  src/osdmenu_module_probe.c
  ${M_OSD_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_M.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_M.ELF PRIVATE -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_M_LAUNCH.ELF
  src/osdmenu_module_probe_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_M_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_M_LAUNCH.ELF PRIVATE -Os -Wall -Wextra -Werror)

# DIAG N: read-only OSDMenu Quickboot fopen / CNF parse, after HDD ELF load.
# Reuses hardware-proven DIAG M IOP modules. Previous binaries untouched.
add_executable(PSX_DVR_HDD_DIAG_N.ELF
  src/osdmenu_quickboot_probe.c
  ${M_OSD_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_N.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_N.ELF PRIVATE -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_N_LAUNCH.ELF
  src/osdmenu_quickboot_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_N_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_N_LAUNCH.ELF PRIVATE -Os -Wall -Wextra -Werror)

# DIAG O: handlePFS read-only cycle & :pfs: loader path, no target execution.
# Reuses physically validated M/N module stack. Prior binaries unmodified.
add_executable(PSX_DVR_HDD_DIAG_O.ELF
  src/osdmenu_handlepfs_probe.c
  ${M_OSD_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_O.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_O.ELF PRIVATE -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_O_LAUNCH.ELF
  src/osdmenu_handlepfs_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_O_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_O_LAUNCH.ELF PRIVATE -Os -Wall -Wextra -Werror)

# DIAG P: OSDMenu-like second IOP reset after Quickboot and before handlePFS.
# Shares verified M/N/O embedded modules; read-only PP.TESTDVR probe only.
add_executable(PSX_DVR_HDD_DIAG_P.ELF
  src/osdmenu_second_reset_probe.c
  ${M_OSD_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_P.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_P.ELF PRIVATE -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_P_LAUNCH.ELF
  src/osdmenu_second_reset_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_P_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_P_LAUNCH.ELF PRIVATE -Os -Wall -Wextra -Werror)

# DIAG Q: executes existing real OSDMenu BOOT.ELF with a strict alternate
# DIAG_Q.CNF pointing to validated standalone F (no Neutrino/game target).
# Only Q launcher is new: the real OSDMenu and DIAG F already reside on HDD.
add_executable(PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF
  src/osdmenu_real_loader_q.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG Q: the *existing* original OSDMenu BOOT.ELF handles real Quickboot
# and its embedded ELF loader. No new HDD target ELF; target is verified F.
add_executable(PSX_DVR_HDD_DIAG_Q.ELF
  src/osdmenu_real_loader_f_probe.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_Q.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_Q.ELF PRIVATE -Os -Wall -Wextra -Werror)

# DIAG R: original OSDMenu loads a harmless argv-echo target using F's
# proven IOP/pad stack. Dummy ISO argument is NEVER opened by the target.
add_executable(PSX_DVR_HDD_DIAG_R_ARG.ELF
  src/osdmenu_arg_echo_target.c
  ${EXIT_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_R_ARG.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_R_ARG.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_R_LAUNCH.ELF
  src/osdmenu_arg_echo_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_R_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_R_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG R2: instrument exact byte differences in DIAG_R.CNF.
# This standalone read-only probe has no LoadELFFromFile path at all.
add_executable(PSX_DVR_HDD_DIAG_R2_CNF.ELF
  src/osdmenu_r2_cnf_byte_probe.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_R2_CNF.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_R2_CNF.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG R3: exact prefix; last LF or NUL only
add_executable(PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF
  src/osdmenu_arg_echo_r3_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG S: no target chainload; read-only Neutrino local config candidate check
# and ISO9660 PVD read at LBA 16 after proven DVR driver preflight.
add_executable(PSX_DVR_HDD_DIAG_S.ELF
  src/neutrino_boot_prereq_probe.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_S.ELF
  pad patches fileXio cdvd libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_S.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG T: read-only target-entry cwd/config/modules visibility via actual OSDMenu.
# Target MUST probe BEFORE its own IOP reset; only then load pad for results.
add_executable(PSX_DVR_HDD_DIAG_T_TARGET.ELF
  src/osdmenu_entry_file_access_target.c
  ${EXIT_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_T_TARGET.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_T_TARGET.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_T_LAUNCH.ELF
  src/osdmenu_entry_file_access_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_T_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_T_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG U: original OSDMenu target-entry read-only remount of PP.TESTDVR
# only AFTER confirming pfs0 is absent and hdd0 is reachable.
add_executable(PSX_DVR_HDD_DIAG_U_TARGET.ELF
  src/osdmenu_entry_remount_target.c
  ${EXIT_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_U_TARGET.ELF
  pad patches fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_U_TARGET.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

add_executable(PSX_DVR_HDD_DIAG_U_LAUNCH.ELF
  src/osdmenu_entry_remount_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_DIAG_U_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_U_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# Probe V: the actual source-based Neutrino diagnostic is built separately
# from pinned pcm720/neutrino:bsd-dvr. This target is ONLY the original
# OSDMenu x-gated launcher using an exact alternate CNF for that test.
add_executable(PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF
  neutrino-hdd-probe/osdmenu_neutrino_v_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# Neutrino V2: same early-stop fork source as V, but target ELF is UNPACKED.
# The separate OSDMenu launcher uses strict DIAG_V2.CNF.
add_executable(PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF
  neutrino-hdd-probe/osdmenu_neutrino_v2_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

# DIAG W: same RO Neutrino V stage-70-safe source, with mandatory
# DelayThread entry marker to distinguish pre-main exit from libc sleep issue.
add_executable(PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF
  neutrino-hdd-probe/osdmenu_neutrino_w_launcher.c
  ${G_DIAG_EMBEDDED})
target_link_libraries(PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF
  pad patches fileXio cdvd elf-loader libdebug.a)
target_compile_options(PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF PRIVATE
  -Os -Wall -Wextra -Werror)

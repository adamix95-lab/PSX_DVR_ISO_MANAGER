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

  # Experimental v1.2: separate executable and hard-isolated PP.DVRISO12.
  # EXCLUDE_FROM_ALL: cannot accidentally replace the stable 1.0 binary.
  # This target is deliberately NOT included in any 1.0 release packager.
  add_executable(DVR_ISO_MANAGER_V12_HDD.ELF EXCLUDE_FROM_ALL ${SOURCES} ${HDD_PFS_SOURCES}
    src/net/v12_https_policy.c src/net/v12_https_client.c
    src/net/v12_net_config.c src/net/v12_tls_clock.c
    src/net/v12_ntp.c src/net/v12_roughtime_wire.c)
  target_compile_definitions(DVR_ISO_MANAGER_V12_HDD.ELF PRIVATE DESR_HDD_TEST=1 DESR_HDD_NETFIX=1 DESR_HDD_PREVIEW=1 DESR_HDD_V12=1)
  target_include_directories(DVR_ISO_MANAGER_V12_HDD.ELF PRIVATE include "$ENV{PS2SDK}/ports/include")
  target_link_libraries(DVR_ISO_MANAGER_V12_HDD.ELF pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
  target_compile_options(DVR_ISO_MANAGER_V12_HDD.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(DVR_ISO_MANAGER_V12_HDD.ELF PRIVATE "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_HDD_NETFIX.map")
endif()

# Non-distributed ABI/link check for wolfSSL on the PS2 EE compiler.
# Explicit opt-in only. This does not ship in 1.2 and is never an HTTPS
# handshake or proof that entropy / RTC / CA trust is correctly configured.
option(DESR_V12_WOLFSSL_LINK_AUDIT "Enable build-only wolfSSL EE API test" OFF)
if(DESR_V12_WOLFSSL_LINK_AUDIT)
  add_executable(V12_WOLFSSL_LINK_AUDIT.ELF EXCLUDE_FROM_ALL src/v12_wolfssl_link_audit.c)
  target_compile_definitions(V12_WOLFSSL_LINK_AUDIT.ELF PRIVATE WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
  target_include_directories(V12_WOLFSSL_LINK_AUDIT.ELF PRIVATE "$ENV{PS2SDK}/ports/include")
  target_link_libraries(V12_WOLFSSL_LINK_AUDIT.ELF "$ENV{PS2SDK}/ports/lib/libwolfssl.a" m)
  target_compile_options(V12_WOLFSSL_LINK_AUDIT.ELF PRIVATE -Os -Wall -Wextra)
endif()

# EXPERIMENTAL Ed25519-only Roughtime ABI audit. A separate wolfSSL port is
# built by CI with Ed25519=yes and SHA512=yes into V12_WOLFSSL_DIR. This must
# NOT change/rebuild the pinned NETFIX SDK or link into the v1.0/v1.2 Manager.
option(DESR_V12_ROUGHTIME_CRYPTO_AUDIT "Build isolated wolfSSL Ed25519 link check" OFF)
if(DESR_V12_ROUGHTIME_CRYPTO_AUDIT)
  if("$ENV{V12_WOLFSSL_DIR}" STREQUAL "")
    message(FATAL_ERROR "Set V12_WOLFSSL_DIR to isolated wolfSSL build install prefix")
  endif()
  if("$ENV{V12_RT_FIXTURE_DIR}" STREQUAL "")
    message(FATAL_ERROR "Set V12_RT_FIXTURE_DIR for public offline Roughtime test-vector header")
  endif()
  add_executable(V12_ROUGHTIME_CRYPTO_AUDIT.ELF EXCLUDE_FROM_ALL
    src/v12_roughtime_crypto_link_audit.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_wolfcrypt.c)
  target_compile_definitions(V12_ROUGHTIME_CRYPTO_AUDIT.ELF PRIVATE WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
  target_include_directories(V12_ROUGHTIME_CRYPTO_AUDIT.ELF PRIVATE
    include "$ENV{V12_RT_FIXTURE_DIR}" "$ENV{V12_WOLFSSL_DIR}/include")
  target_link_libraries(V12_ROUGHTIME_CRYPTO_AUDIT.ELF PRIVATE "$ENV{V12_WOLFSSL_DIR}/lib/libwolfssl.a" m)
  target_compile_options(V12_ROUGHTIME_CRYPTO_AUDIT.ELF PRIVATE -Os -Wall -Wextra)
endif()

# N4B: opt-in wolfSSL callback ABI test. Build only if the *library*
# was itself compiled with WC_RNG_SEED_CB. Never ship as an ELF artifact.
# No TLS, no CA, no network and no RTC/HDD access.
option(DESR_V12_N4B_WOLFSSL_LINK_AUDIT "Audit wolfSSL RNG seed callback ABI" OFF)
if(DESR_V12_N4B_WOLFSSL_LINK_AUDIT)
  add_executable(V12_N4B_WOLFSSL_LINK_AUDIT.ELF EXCLUDE_FROM_ALL
    src/v12_n4b_wolfssl_link_audit.c src/net/v12_n4b_seed_gate.c)
  target_compile_definitions(V12_N4B_WOLFSSL_LINK_AUDIT.ELF PRIVATE
    WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
  target_include_directories(V12_N4B_WOLFSSL_LINK_AUDIT.ELF PRIVATE
    include "$ENV{PS2SDK}/ports/include")
  target_link_libraries(V12_N4B_WOLFSSL_LINK_AUDIT.ELF PRIVATE
    "$ENV{PS2SDK}/ports/lib/libwolfssl.a" m)
  target_compile_options(V12_N4B_WOLFSSL_LINK_AUDIT.ELF PRIVATE
    -Os -Wall -Wextra -Werror)
endif()

# N4B isolated wolfSSL callback test: link only against privately rebuilt
# wolfSSL 5.8.2 with WC_RNG_SEED_CB. Do not overwrite system PS2SDK ports.
option(DESR_V12_N4B_ISOLATED_RNG_AUDIT "N4B isolated callback ABI audit" OFF)
if(DESR_V12_N4B_ISOLATED_RNG_AUDIT)
  if("$ENV{V12_N4B_WOLFSSL_DIR}" STREQUAL "" OR
     NOT EXISTS "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a")
    message(FATAL_ERROR "Set V12_N4B_WOLFSSL_DIR to isolated callback-enabled wolfSSL install")
  endif()
  add_executable(V12_N4B_ISOLATED_RNG_AUDIT.ELF EXCLUDE_FROM_ALL
    src/v12_n4b_wolfssl_link_audit.c src/net/v12_n4b_seed_gate.c)
  target_compile_definitions(V12_N4B_ISOLATED_RNG_AUDIT.ELF PRIVATE
    WC_RNG_SEED_CB=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
  target_include_directories(V12_N4B_ISOLATED_RNG_AUDIT.ELF PRIVATE
    include "$ENV{V12_N4B_WOLFSSL_DIR}/include")
  target_link_libraries(V12_N4B_ISOLATED_RNG_AUDIT.ELF PRIVATE
    "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a" m)
  target_compile_options(V12_N4B_ISOLATED_RNG_AUDIT.ELF PRIVATE
    -Os -Wall -Wextra -Werror)
endif()
# N3B standalone crypto verifier using permissively licensed Monocypher.
# No WolfSSL or network code is linked, and no HDD modules are initialized.
# The source tree is checked out by the dedicated CI workflow with a pinned
# commit; this is EXCLUDE_FROM_ALL and cannot change production targets.
option(DESR_V12_MONOCYPHER_OFFLINE "Build standalone BSD/CC0 Ed25519 N3B test" OFF)
if(DESR_V12_MONOCYPHER_OFFLINE)
  if("$ENV{V12_MONOCYPHER_SRC}" STREQUAL "" OR
     "$ENV{V12_RT_FIXTURE_DIR}" STREQUAL "")
    message(FATAL_ERROR "Set V12_MONOCYPHER_SRC and V12_RT_FIXTURE_DIR")
  endif()
  add_executable(V12_N3B_OFFLINE.ELF EXCLUDE_FROM_ALL
    src/v12_roughtime_offline_diag.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c")
  target_compile_definitions(V12_N3B_OFFLINE.ELF PRIVATE V12_PS2_RUNTIME=1)
  target_include_directories(V12_N3B_OFFLINE.ELF PRIVATE
    include "$ENV{V12_RT_FIXTURE_DIR}"
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N3B_OFFLINE.ELF PRIVATE libdebug.a m)
  target_compile_options(V12_N3B_OFFLINE.ELF PRIVATE -Os -Wall -Wextra)
endif()

# Two strictly opt-in N3C PFS diagnostics. Both are independent of the
# 1.0/1.2 Manager, no network and no secret seeds.
# WRITE_ONCE may create only two new dummy 64-byte files on PP.DVRISO12;
# VERIFY_RO mounts only READ-ONLY for a separate normal-reboot test.
option(DESR_V12_N3C_PFS_DIAG "Enable standalone N3C dummy PFS diagnostic" OFF)
if(DESR_V12_N3C_PFS_DIAG)
  set(N3C_PFS_IRX
    "${CMAKE_CURRENT_BINARY_DIR}/iomanX_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/fileXio_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/dev9_ns_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2atad_hdd_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2hdd_hdd_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2fs_hdd_irx.c")
  foreach(target V12_N3C_PFS_WRITE_ONCE.ELF V12_N3C_PFS_VERIFY_RO.ELF)
    add_executable(${target} EXCLUDE_FROM_ALL
      src/v12_n3c_pfs_probe.c src/net/v12_rt_journal.c ${N3C_PFS_IRX})
    target_include_directories(${target} PRIVATE include)
    target_link_libraries(${target} PRIVATE fileXio cdvd patches libdebug.a m)
    target_compile_options(${target} PRIVATE -Os -Wall -Wextra)
  endforeach()
  target_compile_definitions(V12_N3C_PFS_WRITE_ONCE.ELF PRIVATE V12_N3C_WRITE_ONCE=1)
  target_compile_definitions(V12_N3C_PFS_VERIFY_RO.ELF PRIVATE V12_N3C_VERIFY_RO=1)
endif()

# An explicitly opt-in, read-only N3B live-signature diagnostic.
# Strong nonce/freshness, authenticated time and TLS are NOT enabled.
option(DESR_V12_ROUGHTIME_LIVE_DIAG "Build read-only N3B live signature probe" OFF)
if(DESR_V12_ROUGHTIME_LIVE_DIAG)
  if(NOT DESR_BUILD_HDD_NETFIX)
    message(FATAL_ERROR "N3B live probe requires pinned NETFIX")
  endif()
  if("$ENV{V12_MONOCYPHER_SRC}" STREQUAL "")
    message(FATAL_ERROR "Set V12_MONOCYPHER_SRC to pinned Monocypher checkout")
  endif()
  add_executable(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF EXCLUDE_FROM_ALL
    ${SOURCES} ${HDD_PFS_SOURCES}
    src/net/v12_https_policy.c src/net/v12_https_client.c
    src/net/v12_net_config.c src/net/v12_tls_clock.c
    src/net/v12_ntp.c src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_live_sig.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c")
  target_compile_definitions(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF PRIVATE
    DESR_HDD_TEST=1 DESR_HDD_NETFIX=1 DESR_HDD_PREVIEW=1
    DESR_HDD_V12=1 DESR_V12_RT_LIVE_SIG=1)
  target_include_directories(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF PRIVATE
    include "$ENV{PS2SDK}/ports/include"
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF PRIVATE
    pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a
    "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m)
  target_compile_options(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(DVR_ISO_MANAGER_V12_N3B_LIVE.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N3B_LIVE_NETFIX.map")
endif()

# N3B boot regression: small standalone read-only console diagnostic.
# Excludes Manager UI/ART/ISO code and unused embedded IRX.
option(DESR_V12_N3B_MINI "Build standalone N3B stages + live signature probe" OFF)
if(DESR_V12_N3B_MINI)
  if(NOT DESR_BUILD_HDD_NETFIX)
    message(FATAL_ERROR "N3B MINI needs patched NETFIX PS2SDK")
  endif()
  if("$ENV{V12_MONOCYPHER_SRC}" STREQUAL "")
    message(FATAL_ERROR "Set V12_MONOCYPHER_SRC to pinned Monocypher checkout")
  endif()
  set(N3B_MINI_IRX
    "${CMAKE_CURRENT_BINARY_DIR}/iomanX_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/fileXio_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/dev9_ns_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2atad_hdd_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2hdd_hdd_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/ps2fs_hdd_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/netman_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/smap_irx.c")
  add_executable(V12_N3B_MINI_RO.ELF EXCLUDE_FROM_ALL
    src/v12_n3b_standalone.c
    src/net/v12_net_config.c
    src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_live_sig.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
    ${N3B_MINI_IRX})
  target_include_directories(V12_N3B_MINI_RO.ELF PRIVATE
    include
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N3B_MINI_RO.ELF PRIVATE
    fileXio cdvd patches ps2ip netman libdebug.a m)
  target_compile_options(V12_N3B_MINI_RO.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(V12_N3B_MINI_RO.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N3B_MINI_RO.map")
  # Alternative PINNED Roughtime server, compiled separately from
  # Cloudflare and from the full Manager. Zero PFS writes, no TLS clock.
  add_executable(V12_N3B_TXRYAN_RO.ELF EXCLUDE_FROM_ALL
    src/v12_n3b_standalone.c
    src/net/v12_net_config.c
    src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_txryan_sig.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
    ${N3B_MINI_IRX})
  target_compile_definitions(V12_N3B_TXRYAN_RO.ELF PRIVATE V12_N3B_TXRYAN=1)
  target_include_directories(V12_N3B_TXRYAN_RO.ELF PRIVATE
    include
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N3B_TXRYAN_RO.ELF PRIVATE
    fileXio cdvd patches ps2ip netman libdebug.a m)
  target_compile_options(V12_N3B_TXRYAN_RO.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(V12_N3B_TXRYAN_RO.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N3B_TXRYAN_RO.map")
  # N3D USB nonce: separate read-only binary, not built by default.
  # No production Manager, PFS or XMB changes.
  set(N3D_USB_IRX ${N3B_MINI_IRX}
    "${CMAKE_CURRENT_BINARY_DIR}/bdm_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/bdmfs_fatfs_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/usbd_mini_irx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/usbmass_bd_mini_irx.c")
  add_executable(V12_N3D_TXRYAN_USB_RO.ELF EXCLUDE_FROM_ALL
    src/v12_n3d_txryan_usb.c
    src/net/v12_net_config.c
    src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_roughtime_txryan_sig.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
    ${N3D_USB_IRX})
  target_compile_definitions(V12_N3D_TXRYAN_USB_RO.ELF PRIVATE V12_N3B_TXRYAN=1)
  target_include_directories(V12_N3D_TXRYAN_USB_RO.ELF PRIVATE include
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N3D_TXRYAN_USB_RO.ELF PRIVATE
    fileXio cdvd patches ps2ip netman libdebug.a m)
  target_compile_options(V12_N3D_TXRYAN_USB_RO.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(V12_N3D_TXRYAN_USB_RO.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N3D_TXRYAN_USB_RO.map")
  # N3E read-only signed UTC diagnostic, using the physical N3D boot/USB path.
  # Time remains diagnostic in RAM only; no wolfSSL clock or RTC writes.
  add_executable(V12_N3E_TXRYAN_UTC_RO.ELF EXCLUDE_FROM_ALL
    src/v12_n3e_txryan_usb.c
    src/net/v12_net_config.c
    src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_n3e_txryan.c
    src/net/v12_n3e_utc.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
    ${N3D_USB_IRX})
  target_include_directories(V12_N3E_TXRYAN_UTC_RO.ELF PRIVATE include
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N3E_TXRYAN_UTC_RO.ELF PRIVATE
    fileXio cdvd patches ps2ip netman libdebug.a m)
  target_compile_options(V12_N3E_TXRYAN_UTC_RO.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(V12_N3E_TXRYAN_UTC_RO.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N3E_TXRYAN_UTC_RO.map")


  # N4A: only read-only dual-domain host entropy and signed UTC input audit.
  # No wolfSSL, TLS session, CA, RTC, cover/ART or storage writes.
  add_executable(V12_N4A_ENTROPY_RO.ELF EXCLUDE_FROM_ALL
    src/v12_n4a_entropy_usb.c
    src/net/v12_n4a_entropy.c
    src/net/v12_net_config.c
    src/net/v12_roughtime_wire.c
    src/net/v12_roughtime_verify.c
    src/net/v12_n3e_txryan.c
    src/net/v12_n3e_utc.c
    src/net/v12_roughtime_monocypher.c
    "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
    ${N3D_USB_IRX})
  target_include_directories(V12_N4A_ENTROPY_RO.ELF PRIVATE include
    "$ENV{V12_MONOCYPHER_SRC}/src"
    "$ENV{V12_MONOCYPHER_SRC}/src/optional")
  target_link_libraries(V12_N4A_ENTROPY_RO.ELF PRIVATE
    fileXio cdvd patches ps2ip netman libdebug.a m)
  target_compile_options(V12_N4A_ENTROPY_RO.ELF PRIVATE -Os -Wall -Wextra)
  target_link_options(V12_N4A_ENTROPY_RO.ELF PRIVATE
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N4A_ENTROPY_RO.map")
  # N4E TLS1.2-only diagnostic: strict pinned TXRyan plus two
  # chain+hostname+certificate-time verified TLS handshakes.
  # Not built by default, no HTTP or storage writes.
  option(DESR_V12_N4E_TLS_RO "Build isolated N4E verified TLS probe" OFF)
  if(DESR_V12_N4E_TLS_RO)
    if("$ENV{V12_N4B_WOLFSSL_DIR}" STREQUAL "" OR
       NOT EXISTS "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a")
      message(FATAL_ERROR "Set V12_N4B_WOLFSSL_DIR to isolated callback/SNI-enabled wolfSSL")
    endif()
    add_executable(V12_N4E_TLS_RO.ELF EXCLUDE_FROM_ALL
      src/v12_n4e_tls_probe.c
      src/net/v12_net_config.c
      src/net/v12_roughtime_wire.c
      src/net/v12_roughtime_verify.c
      src/net/v12_n3e_txryan.c
      src/net/v12_n3e_utc.c
      src/net/v12_n4a_entropy.c
      src/net/v12_n4b_seed_gate.c
      src/net/v12_roughtime_monocypher.c
      "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
      ${N3D_USB_IRX})
    target_compile_definitions(V12_N4E_TLS_RO.ELF PRIVATE
      WC_RNG_SEED_CB=1 HAVE_SNI=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
    target_include_directories(V12_N4E_TLS_RO.ELF PRIVATE include
      "$ENV{V12_N4B_WOLFSSL_DIR}/include"
      "$ENV{V12_MONOCYPHER_SRC}/src"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional")
    target_link_libraries(V12_N4E_TLS_RO.ELF PRIVATE
      fileXio cdvd patches ps2ip netman
      "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a"
      libdebug.a m)
    target_compile_options(V12_N4E_TLS_RO.ELF PRIVATE -Os -Wall -Wextra)
    target_link_options(V12_N4E_TLS_RO.ELF PRIVATE
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N4E_TLS_RO.map")
    # Separate failure screen for physical RET=-649; same strict verification.
    add_executable(V12_N4E_NETDIAG_RO.ELF EXCLUDE_FROM_ALL
      src/v12_n4e_tls_netdiag.c
      src/net/v12_net_config.c
      src/net/v12_roughtime_wire.c
      src/net/v12_roughtime_verify.c
      src/net/v12_n3e_txryan.c
      src/net/v12_n3e_utc.c
      src/net/v12_n4a_entropy.c
      src/net/v12_n4b_seed_gate.c
      src/net/v12_roughtime_monocypher.c
      "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
      ${N3D_USB_IRX})
    target_compile_definitions(V12_N4E_NETDIAG_RO.ELF PRIVATE
      WC_RNG_SEED_CB=1 HAVE_SNI=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
    target_include_directories(V12_N4E_NETDIAG_RO.ELF PRIVATE include
      "$ENV{V12_N4B_WOLFSSL_DIR}/include"
      "$ENV{V12_MONOCYPHER_SRC}/src"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional")
    target_link_libraries(V12_N4E_NETDIAG_RO.ELF PRIVATE
      fileXio cdvd patches ps2ip netman
      "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a"
      libdebug.a m)
    target_compile_options(V12_N4E_NETDIAG_RO.ELF PRIVATE -Os -Wall -Wextra)
    target_link_options(V12_N4E_NETDIAG_RO.ELF PRIVATE
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N4E_NETDIAG_RO.map")
    # N4E certificate signature trace, same VERIFY_PEER and SSL clock.
    add_executable(V12_N4E_SIGTRACE_RO.ELF EXCLUDE_FROM_ALL
      src/v12_n4e_tls_sigtrace.c
      src/net/v12_net_config.c
      src/net/v12_roughtime_wire.c
      src/net/v12_roughtime_verify.c
      src/net/v12_n3e_txryan.c
      src/net/v12_n3e_utc.c
      src/net/v12_n4a_entropy.c
      src/net/v12_n4b_seed_gate.c
      src/net/v12_roughtime_monocypher.c
      "$ENV{V12_MONOCYPHER_SRC}/src/monocypher.c"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional/monocypher-ed25519.c"
      ${N3D_USB_IRX})
    target_compile_definitions(V12_N4E_SIGTRACE_RO.ELF PRIVATE
      WC_RNG_SEED_CB=1 HAVE_SNI=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
    target_include_directories(V12_N4E_SIGTRACE_RO.ELF PRIVATE include
      "$ENV{V12_N4B_WOLFSSL_DIR}/include"
      "$ENV{V12_MONOCYPHER_SRC}/src"
      "$ENV{V12_MONOCYPHER_SRC}/src/optional")
    target_link_libraries(V12_N4E_SIGTRACE_RO.ELF PRIVATE
      fileXio cdvd patches ps2ip netman
      "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a"
      libdebug.a m)
    target_compile_options(V12_N4E_SIGTRACE_RO.ELF PRIVATE -Os -Wall -Wextra)
    target_link_options(V12_N4E_SIGTRACE_RO.ELF PRIVATE
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N4E_SIGTRACE_RO.map")
  endif()
  # N4D physical wolfCrypt RNG-only diagnostic; no TLS and no writes.
  # Link against isolated callback-enabled wolfSSL, never stock lib.
  option(DESR_V12_N4D_WOLFCRYPT_TEST "Build isolated N4D DRBG diagnostic" OFF)
  if(DESR_V12_N4D_WOLFCRYPT_TEST)
    if("$ENV{V12_N4B_WOLFSSL_DIR}" STREQUAL "" OR
       NOT EXISTS "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a")
      message(FATAL_ERROR "Provide isolated callback-enabled V12_N4B_WOLFSSL_DIR")
    endif()
    add_executable(V12_N4D_WOLFCRYPT_RNG_RO.ELF EXCLUDE_FROM_ALL
      src/v12_n4d_wolfcrypt_rng_ro.c
      src/net/v12_n4b_seed_gate.c
      ${N3D_USB_IRX})
    target_compile_definitions(V12_N4D_WOLFCRYPT_RNG_RO.ELF PRIVATE
      WC_RNG_SEED_CB=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1 NO_WRITEV=1)
    target_include_directories(V12_N4D_WOLFCRYPT_RNG_RO.ELF PRIVATE
      include "$ENV{V12_N4B_WOLFSSL_DIR}/include")
    target_link_libraries(V12_N4D_WOLFCRYPT_RNG_RO.ELF PRIVATE
      fileXio cdvd patches "$ENV{V12_N4B_WOLFSSL_DIR}/lib/libwolfssl.a"
      libdebug.a m)
    target_compile_options(V12_N4D_WOLFCRYPT_RNG_RO.ELF PRIVATE -Os -Wall -Wextra)
    target_link_options(V12_N4D_WOLFCRYPT_RNG_RO.ELF PRIVATE
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/V12_N4D_WOLFCRYPT_RNG_RO.map")
  endif()

endif()

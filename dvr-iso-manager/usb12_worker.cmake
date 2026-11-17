set(wolfssl "$ENV{V12_N4B_WOLFSSL_DIR}")
set(monocypher "$ENV{V12_MONOCYPHER_SRC}/src")
set(fat_irx "$ENV{V12_N6G_FAT_IRX}")
if(NOT EXISTS "$ENV{PS2SDK}/HDD_NETFIX_BUILD.txt" OR
   NOT EXISTS "${wolfssl}/lib/libwolfssl.a" OR
   NOT EXISTS "${monocypher}/optional/monocypher-ed25519.c" OR
   NOT EXISTS "${fat_irx}")
  message(FATAL_ERROR "USB 1.2 worker needs NETFIX PS2SDK, pinned wolfSSL/Monocypher and N6G FAT IRX")
endif()

set(fat_source "${CMAKE_CURRENT_BINARY_DIR}/n6g_bdmfs_fatfs_irx.c")
add_custom_command(OUTPUT "${fat_source}"
  COMMAND "$ENV{PS2SDK}/bin/bin2c" "${fat_irx}" "${fat_source}" bdmfs_fatfs_irx
  DEPENDS "${fat_irx}" VERBATIM)

# Same source and IRX order as the DESR-7000-tested N6G BATCH50 R3.
set(build "${CMAKE_CURRENT_BINARY_DIR}")
set(target COVER_HTTPS_V12.ELF)
add_executable(${target} EXCLUDE_FROM_ALL
  src/v12_n6f_https_usb_art_one.c
  src/net/n6_usb_art.c
  src/net/n6_normalize_png.c
  src/cover_index.c
  src/iso_utils.c
  src/net/probe_png.c
  src/net/v12_net_config.c
  src/net/v12_roughtime_wire.c
  src/net/v12_roughtime_verify.c
  src/net/v12_n3e_txryan.c
  src/net/v12_n3e_utc.c
  src/net/v12_n4a_entropy.c
  src/net/v12_n4b_seed_gate.c
  src/net/v12_roughtime_monocypher.c
  "${monocypher}/monocypher.c"
  "${monocypher}/optional/monocypher-ed25519.c"
  "${build}/iomanX_irx.c"
  "${build}/fileXio_irx.c"
  "${build}/dev9_ns_irx.c"
  "${build}/ps2atad_hdd_irx.c"
  "${build}/ps2hdd_hdd_irx.c"
  "${build}/ps2fs_hdd_irx.c"
  "${build}/netman_irx.c"
  "${build}/smap_irx.c"
  "${build}/bdm_irx.c"
  "${fat_source}"
  "${build}/usbd_mini_irx.c"
  "${build}/usbmass_bd_mini_irx.c"
  "${build}/dvrdrv_irx.c"
  "${build}/dvrfile_irx.c"
  src/net/n6g_tls_arena.c
  src/net/n6g_batch_entropy.c)
target_compile_definitions(${target} PRIVATE
  DESR_USB12_HTTPS=1 N6G_BATCH_50=1 N6G_TLS_ARENA=1 N6G_PNG_WORKSPACE=1 N6G_USB_SAFE=1
  WC_RNG_SEED_CB=1 HAVE_SNI=1 WOLFSSL_USER_IO=1 WOLFSSL_NO_SOCK=1
  NO_WRITEV=1 USE_FAST_MATH=1 TFM_NO_ASM=1 FP_MAX_BITS=8192)
option(DESR_USB12_EMBEDDED_WORKER "Return to the single-file Manager" OFF)
if(DESR_USB12_EMBEDDED_WORKER)
  target_compile_definitions(${target} PRIVATE DESR_USB12_ONE_ELF=1)
endif()
if(DESR_USB12_USE_CPU_ENTROPY)
  if(NOT DESR_USB12_EMBEDDED_WORKER)
    message(FATAL_ERROR "CPU counter experiment needs the embedded worker")
  endif()
  target_compile_definitions(${target} PRIVATE DESR_USB12_CPU_ENTROPY=1)
  target_sources(${target} PRIVATE src/net/n6g_cpu_entropy.c)
endif()
target_include_directories(${target} PRIVATE include
  "${wolfssl}/include" "${monocypher}" "${monocypher}/optional")
target_link_libraries(${target} PRIVATE fileXio cdvd patches ps2ip netman
  "${wolfssl}/lib/libwolfssl.a"
  "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a"
  libdebug.a m elf-loader)
target_compile_options(${target} PRIVATE -Os -Wall -Wextra -fstack-usage)
target_link_options(${target} PRIVATE
  "-Wl,-Map=${build}/COVER_HTTPS_V12.map"
  "-Wl,--wrap=InitSSL" "-Wl,--wrap=wc_InitRng_ex")

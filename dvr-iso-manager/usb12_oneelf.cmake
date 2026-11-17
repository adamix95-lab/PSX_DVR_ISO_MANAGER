set(https_worker "$ENV{DESR_USB12_PAYLOAD}")
set(loader_header "$ENV{DESR_USB12_ELF_LOADER2_HEADER_DIR}")
set(loader_library "$ENV{DESR_USB12_ELF_LOADER2_LIBRARY}")
if(NOT EXISTS "${https_worker}" OR
   NOT EXISTS "${loader_header}/elf_loader_common.h" OR
   NOT EXISTS "${loader_library}")
  message(FATAL_ERROR "USB 1.2 single ELF needs the worker and pinned PS2SDK elf-loader2")
endif()

enable_language(ASM)
set(payload_asm "${CMAKE_CURRENT_BINARY_DIR}/usb12_payload.S")
configure_file(src/usb12_payload.S.in "${payload_asm}" @ONLY)
set_source_files_properties("${payload_asm}" PROPERTIES OBJECT_DEPENDS "${https_worker}")

set(single DVR_ISO_MANAGER_V12_SINGLE.ELF)
if(DESR_USB12_USE_CPU_ENTROPY)
  set(single DVR_ISO_MANAGER_V12_CPU.ELF)
endif()
add_executable(${single} EXCLUDE_FROM_ALL ${SOURCES} src/usb12_launch_embedded.c "${payload_asm}")
target_compile_definitions(${single} PRIVATE DESR_USB12_HTTPS=1 DESR_USB12_ONE_ELF=1)
if(DESR_USB12_USE_CPU_ENTROPY)
  target_compile_definitions(${single} PRIVATE DESR_USB12_CPU_ENTROPY=1)
endif()
target_include_directories(${single} PRIVATE include "${loader_header}" "$ENV{PS2SDK}/ports/include")
target_link_libraries(${single} PRIVATE pad patches fileXio cdvd elf-loader ps2ip netman libdebug.a
  "$ENV{PS2SDK}/ports/lib/libpng.a" "$ENV{PS2SDK}/ports/lib/libz.a" m "${loader_library}")
target_compile_options(${single} PRIVATE -Os -Wall -Wextra)

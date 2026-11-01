cmake_minimum_required(VERSION 3.13)
include($ENV{PS2SDK}/ps2dev.cmake)
project(psx_dvr_hdd_diagnostic C)

# Stage A attaches to wLaunchELF R3Z's already initialized IOP stack.
# No IRX is embedded or loaded, no HDD operations can write data.
add_executable(PSX_DVR_HDD_DIAG_A.ELF src/main.c)
target_link_libraries(PSX_DVR_HDD_DIAG_A.ELF fileXio libdebug.a)
target_compile_options(PSX_DVR_HDD_DIAG_A.ELF PRIVATE -Os -Wall -Wextra -Werror)

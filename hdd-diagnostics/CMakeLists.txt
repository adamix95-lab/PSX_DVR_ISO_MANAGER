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

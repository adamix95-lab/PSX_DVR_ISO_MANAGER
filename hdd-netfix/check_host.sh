#!/bin/sh
set -eu
task_tmp=$(mktemp -d)
trap 'rm -rf "$task_tmp"' EXIT
inc=dvr-iso-manager/include
src=dvr-iso-manager/src
tests=dvr-iso-manager/tests
for name in iso_utils cover_color; do
  cc -std=c99 -Wall -Wextra -Werror -I"$inc" "$src/$name.c" "$tests/test_$name.c" -o "$task_tmp/$name"
  "$task_tmp/$name"
done
cc -std=c99 -Wall -Wextra -Werror -I"$inc" "$src/iso_utils.c" "$src/cover_index.c" "$tests/test_cover_index.c" -o "$task_tmp/index"
"$task_tmp/index"
cc -std=c99 -Wall -Wextra -Werror -I"$inc" "$src/iso_utils.c" "$src/cover_index.c" "$src/cover_report.c" "$tests/test_cover_report.c" -o "$task_tmp/report"
"$task_tmp/report"
cc -std=c99 -Wall -Wextra -Werror -I"$inc" "$src/iso_utils.c" "$src/hdd_boot_config.c" "$tests/test_hdd_boot_config.c" -o "$task_tmp/cnf"
"$task_tmp/cnf"
for name in probe_config probe_png art_sync; do
  cc -std=c99 -Wall -Wextra -Werror -I"$inc" "$src/net/$name.c" "$tests/net/test_$name.c" -o "$task_tmp/$name"
  "$task_tmp/$name"
done

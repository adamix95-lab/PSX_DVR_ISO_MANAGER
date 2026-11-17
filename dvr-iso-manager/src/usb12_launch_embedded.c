#include "usb12_covers.h"

#include <elf_loader_common.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdint.h>

#ifdef DESR_USB12_WORKER_LOADER
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#else
#ifdef DESR_USB12_CPU_ENTROPY
#include "n6g_cpu_entropy.h"
#include <stdio.h>
#endif
extern const unsigned char desr_usb12_payload_start[];
extern const unsigned char desr_usb12_payload_end[];
#endif

/* elf-loader2 moves this one segment and clears the rest of EE user RAM. */
static int validate_payload(const unsigned char *image, size_t size) {
  const elf_loader_elf32_ehdr_t *header = (const void *)image;
  const elf_loader_elf32_phdr_t *segments, *load = NULL;
  unsigned memory = GetMemorySize();

  if (size < sizeof(*header) || elf_loader_is_elf_ehdr_valid(header) != 0 ||
      (header->e_phoff & 3U) || header->e_phoff > size ||
      header->e_phnum > (size - header->e_phoff) / sizeof(*segments))
    return -1;

  segments = (const void *)(image + header->e_phoff);
  for (unsigned i = 0; i < header->e_phnum; ++i) {
    if (segments[i].p_type != 1) continue;
    if (load) return -2;
    load = &segments[i];
  }
  if (!load || load->p_vaddr != 0x00100000U ||
      load->p_offset > size || load->p_filesz > size - load->p_offset ||
      load->p_filesz > load->p_memsz || load->p_vaddr > memory ||
      load->p_memsz > memory - load->p_vaddr ||
      header->e_entry < load->p_vaddr ||
      header->e_entry - load->p_vaddr >= load->p_filesz)
    return -3;
  return 0;
}

static int run_usb12_elf(const unsigned char *image, size_t size, int argc, char *args[]) {
  elf_loader_execinfo_t exec = {0};
  int rc = validate_payload(image, size);

  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_loadinfo(&exec, image, size);
  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_arginfo(&exec, DESR_USB12_MANAGER_ELF, NULL, argc, args);
  if (rc != 0) return rc;

  while (!SifIopReset(NULL, 0)) { }
  while (!SifIopSync()) { }
  sceSifExitRpc();
  return elf_loader_exec_elf(&exec);
}

#ifdef DESR_USB12_WORKER_LOADER
int desr_usb12_return_to_manager(int argc, char *argv[]) {
  unsigned char *image = NULL;
  int fd = fileXioOpen(DESR_USB12_MANAGER_ELF, FIO_O_RDONLY, 0);
  int size, offset = 0, rc = -736;
  if (fd < 0) return rc;

  size = fileXioLseek(fd, 0, SEEK_END);
  if (size > 0 && size <= 16 * 1024 * 1024 && fileXioLseek(fd, 0, SEEK_SET) == 0) {
    image = memalign(64, (size_t)size);
    if (!image) rc = -737;
    while (image && offset < size) {
      int count = size - offset;
      if (count > 16384) count = 16384;
      int read = fileXioRead(fd, image + offset, count);
      if (read <= 0 || read > count) break;
      offset += read;
    }
  }
  if (fileXioClose(fd) == 0 && image && size == offset)
    rc = run_usb12_elf(image, (size_t)size, argc, argv);
  free(image);
  return rc;
}
#else
int desr_run_embedded_https(char *language) {
  const unsigned char *image = desr_usb12_payload_start;
  size_t size = (uintptr_t)desr_usb12_payload_end - (uintptr_t)image;
#ifdef DESR_USB12_CPU_ENTROPY
  char ticks_arg[9];
  char *args[] = { language, ticks_arg };
  snprintf(ticks_arg, sizeof(ticks_arg), "%08x", (unsigned)n6g_read_cpu_count());
#else
  char *args[] = { language };
#endif
  return run_usb12_elf(image, size, sizeof(args) / sizeof(args[0]), args);
}
#endif

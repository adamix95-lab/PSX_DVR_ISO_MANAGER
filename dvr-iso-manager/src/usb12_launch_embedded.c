#include "usb12_covers.h"

#include <elf_loader_common.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdint.h>

#ifdef DESR_USB12_CPU_ENTROPY
#include "n6g_cpu_entropy.h"
#include <stdio.h>
#endif

#ifdef DESR_USB12_TAIL_PAYLOAD
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <debug.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
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

static int launch_image(const unsigned char *image, size_t size, int argc, char *args[]) {
  elf_loader_execinfo_t exec = {0};
  int rc = validate_payload(image, size);

  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_loadinfo(&exec, image, size);
  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_arginfo(&exec, DESR_USB12_MANAGER_ELF, NULL,
                                         argc, args);
  if (rc != 0) return rc;

  while (!SifIopReset(NULL, 0)) { }
  while (!SifIopSync()) { }
  sceSifExitRpc();
  return elf_loader_exec_elf(&exec);
}

#ifdef DESR_USB12_TAIL_PAYLOAD
/* Footer and payload are appended by tools/pack_usb12_tail.py. Neither is PT_LOAD. */
typedef struct {
  char magic[8];
  uint32_t size;
  uint32_t complement;
} worker_footer;

_Static_assert(sizeof(worker_footer) == 16, "Worker footer size");

static int read_exact(int fd, unsigned char *dest, size_t size) {
  while (size > 0) {
    int chunk = size > 16384U ? 16384 : (int)size;
    int count = fileXioRead(fd, dest, chunk);
    if (count <= 0 || count > chunk) return 0;
    dest += count;
    size -= (size_t)count;
  }
  return 1;
}

static int launch_from_tail(int argc, char *args[]) {
  worker_footer footer;
  unsigned char *image = NULL;
  int fd, file_size, offset, loaded = 0, rc = -736;

  scr_printf("Reading HTTPS worker from Manager ELF...\n");
  fd = fileXioOpen(DESR_USB12_MANAGER_ELF, FIO_O_RDONLY, 0);
  if (fd < 0) return rc;
  file_size = fileXioLseek(fd, 0, SEEK_END);
  if (file_size > (int)sizeof(footer)) {
    offset = file_size - (int)sizeof(footer);
    if (fileXioLseek(fd, offset, SEEK_SET) == offset &&
        read_exact(fd, (unsigned char *)&footer, sizeof(footer)) &&
        memcmp(footer.magic, "N6GCPUR3", sizeof(footer.magic)) == 0 &&
        footer.size == (footer.complement ^ UINT32_MAX) &&
        footer.size > 0 && footer.size <= 8U * 1024U * 1024U &&
        footer.size < (unsigned)offset) {
      offset -= (int)footer.size;
      if (fileXioLseek(fd, offset, SEEK_SET) == offset) {
        image = memalign(64, footer.size);
        if (image) loaded = read_exact(fd, image, footer.size);
        else rc = -737;
      }
    }
  }
  if (fileXioClose(fd) == 0 && loaded)
    rc = launch_image(image, footer.size, argc, args);
  free(image);
  return rc;
}
#endif

int desr_run_embedded_https(char *language) {
#ifdef DESR_USB12_CPU_ENTROPY
  char ticks_arg[9];
  char *args[] = { language, ticks_arg };
  snprintf(ticks_arg, sizeof(ticks_arg), "%08x", (unsigned)n6g_read_cpu_count());
#else
  char *args[] = { language };
#endif
#ifdef DESR_USB12_TAIL_PAYLOAD
  return launch_from_tail(sizeof(args) / sizeof(args[0]), args);
#else
  const unsigned char *image = desr_usb12_payload_start;
  size_t size = (uintptr_t)desr_usb12_payload_end - (uintptr_t)image;
  return launch_image(image, size, sizeof(args) / sizeof(args[0]), args);
#endif
}

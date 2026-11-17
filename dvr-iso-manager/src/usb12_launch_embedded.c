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

enum {
  WORKER_OPEN = -736,
  WORKER_MEMORY = -737,
  WORKER_SIZE = -738,
  WORKER_READ = -739,
  WORKER_FOOTER = -740,
  WORKER_CLOSE = -741
};

const char *desr_usb12_loader_error(int result) {
  switch (result) {
    case WORKER_OPEN: return "OPEN MANAGER ELF";
    case WORKER_MEMORY: return "ALLOCATE EE MEMORY";
    case WORKER_SIZE: return "GET MANAGER FILE SIZE";
    case WORKER_READ: return "READ MANAGER ELF";
    case WORKER_FOOTER: return "HTTPS PAYLOAD FOOTER";
    case WORKER_CLOSE: return "CLOSE MANAGER ELF";
    default: return "PREPARE HTTPS ELF";
  }
}

static int read_exact(int fd, unsigned char *dest, size_t size) {
  while (size > 0) {
    int chunk = size > 4096U ? 4096 : (int)size;
    int count = fileXioRead(fd, dest, chunk);
    if (count <= 0 || count > chunk) return 0;
    dest += count;
    size -= (size_t)count;
  }
  return 1;
}

static int launch_from_tail(int argc, char *args[]) {
  iox_stat_t info __attribute__((aligned(64)));
  worker_footer footer;
  unsigned char *image;
  size_t footer_offset;
  int fd, rc, closed;

#ifndef DESR_USB12_UI_SIMPLE
  scr_printf("Reading HTTPS worker from Manager ELF...\n");
#endif
  if (fileXioGetStat(DESR_USB12_MANAGER_ELF, &info) < 0 ||
      info.hisize != 0 || info.size <= sizeof(footer) || info.size > 16U * 1024U * 1024U)
    return WORKER_SIZE;

  fd = fileXioOpen(DESR_USB12_MANAGER_ELF, FIO_O_RDONLY, 0);
  if (fd < 0) return WORKER_OPEN;
  image = memalign(64, info.size);
  if (!image) {
    fileXioClose(fd);
    return WORKER_MEMORY;
  }
  rc = read_exact(fd, image, info.size) ? 0 : WORKER_READ;
  closed = fileXioClose(fd);
  if (!rc && closed < 0) rc = WORKER_CLOSE;
  if (!rc) {
    footer_offset = (size_t)info.size - sizeof(footer);
    memcpy(&footer, image + footer_offset, sizeof(footer));
    if (memcmp(footer.magic, "N6GCPUR3", sizeof(footer.magic)) != 0 ||
        footer.size != (footer.complement ^ UINT32_MAX) ||
        footer.size < sizeof(elf_loader_elf32_ehdr_t) ||
        footer.size > 8U * 1024U * 1024U || footer.size >= footer_offset) {
      rc = WORKER_FOOTER;
    } else {
      memmove(image, image + footer_offset - footer.size, footer.size);
      rc = launch_image(image, footer.size, argc, args);
    }
  }
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

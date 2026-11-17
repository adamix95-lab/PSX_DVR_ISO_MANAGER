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

extern const unsigned char desr_usb12_payload_start[];
extern const unsigned char desr_usb12_payload_end[];

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

int desr_run_embedded_https(char *language) {
  const unsigned char *image = desr_usb12_payload_start;
  size_t size = (uintptr_t)desr_usb12_payload_end - (uintptr_t)image;
  elf_loader_execinfo_t exec = {0};
#ifdef DESR_USB12_CPU_ENTROPY
  char ticks_arg[9];
  char *args[] = { language, ticks_arg };
  snprintf(ticks_arg, sizeof(ticks_arg), "%08x", (unsigned)n6g_read_cpu_count());
#else
  char *args[] = { language };
#endif
  int rc = validate_payload(image, size);

  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_loadinfo(&exec, image, size);
  if (rc != 0) return rc;
  rc = elf_loader_exec_elf_prepare_arginfo(&exec, DESR_USB12_MANAGER_ELF, NULL,
                                         sizeof(args) / sizeof(args[0]), args);
  if (rc != 0) return rc;

  while (!SifIopReset(NULL, 0)) { }
  while (!SifIopSync()) { }
  sceSifExitRpc();
  return elf_loader_exec_elf(&exec);
}

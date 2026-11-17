#include "n6g_roughtime_retry.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int codes[3];
  unsigned calls;
  uint8_t seen[3][32];
} fixture;

static int fake_probe(const uint8_t nonce[32], void *arg) {
  fixture *f = (fixture *)arg;
  assert(f->calls < N6G_ROUGHTIME_MAX_ATTEMPTS);
  memcpy(f->seen[f->calls], nonce, 32);
  return f->codes[f->calls++];
}

static void scenario(const uint8_t base[32], int a, int b, int c,
                     int expected_calls, int expected_result) {
  fixture f = {{a, b, c}, 0, {{0}}};
  int rc = n6g_roughtime_retry(base, fake_probe, &f);
  assert(rc == expected_result);
  assert(f.calls == (unsigned)expected_calls);
  assert(memcmp(f.seen[0], base, 32) == 0);
  for (unsigned i = 1; i < f.calls; i++) {
    assert(memcmp(f.seen[i], base, 32) != 0);
    for (unsigned j = 1; j < i; j++)
      assert(memcmp(f.seen[i], f.seen[j], 32) != 0);
  }
}

int main(void) {
  uint8_t base[32];
  for (unsigned i = 0; i < 32; i++) base[i] = (uint8_t)(i * 7U + 13U);
  scenario(base, 0, -508, 0, 1, 0);
  scenario(base, -508, 0, -508, 2, 0);
  scenario(base, -507, -508, 0, 3, 0);
  scenario(base, -508, -508, -508, 3, -508);
  scenario(base, -515, -506, 0, 3, 0);
  scenario(base, -649, 0, 0, 1, -649); /* No TLS error retry here. */
  scenario(base, -505, 0, 0, 1, -505); /* No DNS failure masking. */
  assert(n6g_roughtime_retry(NULL, fake_probe, NULL) != 0);
  assert(n6g_roughtime_retry(base, NULL, NULL) != 0);
  puts("PASS: strict Roughtime retries, distinct nonces, fail closed");
  return 0;
}

/* N3B-OFFLINE: standalone source-level test, not part of the Manager.
 * Uses Monocypher BSD-2-Clause/CC0 and a PUBLIC synthetic test vector.
 * No sockets, HTTP, TLS, RTC, APA/PFS calls, secret seed, RNG or disk writes.
 *
 * Prints result to GS debug screen when cross-compiled for PS2, or stdout
 * when compiled as a host test. This proves ONLY offline cryptographic
 * runtime behavior on the console, never live server authenticity.
 */
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include "v12_rt_test_vector.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef V12_PS2_RUNTIME
#include <debug.h>
#include <unistd.h>
#endif

static int run_fixture(void) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time result={0,0},sentinel;
  uint8_t wrongroot[32],wrongnonce[32],tamper[sizeof(rt_fixture_reply)];
  int rc;
  rc=v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
      rt_fixture_root,rt_fixture_nonce,&cb,&result);
  if(rc!=0) return 11;
  if(result.midpoint_utc!=UINT64_C(50) || result.radius_seconds!=5U)
    return 12;

  /* Fail-closed checks also require output to remain UNMODIFIED. */
  memset(wrongroot,0,sizeof(wrongroot));
  memset(wrongnonce,0,sizeof(wrongnonce));
  sentinel.midpoint_utc=UINT64_C(911);
  sentinel.radius_seconds=911U;
  if(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
       wrongroot,rt_fixture_nonce,&cb,&sentinel)==0) return 13;
  if(sentinel.midpoint_utc!=UINT64_C(911)) return 14;
  if(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
       rt_fixture_root,wrongnonce,&cb,&sentinel)==0) return 15;
  if(sentinel.midpoint_utc!=UINT64_C(911)) return 16;
  memcpy(tamper,rt_fixture_reply,sizeof(tamper));
  tamper[64]^=0x01;
  if(v12_rt_verify_draft08(tamper,sizeof(tamper),rt_fixture_root,
       rt_fixture_nonce,&cb,&sentinel)==0) return 17;
  if(sentinel.midpoint_utc!=UINT64_C(911)) return 18;
  if(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply)-1U,
       rt_fixture_root,rt_fixture_nonce,&cb,&sentinel)==0) return 19;
  if(sentinel.midpoint_utc!=UINT64_C(911)) return 20;
  return 0;
}

int main(void) {
  int rc;
#ifdef V12_PS2_RUNTIME
  init_scr();
  scr_printf("PSX DVR ISO MANAGER 1.2 - N3B OFFLINE TEST\n");
  scr_printf("Read-only cryptographic self-test. No network.\n");
  scr_printf("Monocypher Ed25519 + SHA-512 (BSD-2-Clause).\n");
  scr_printf("Public synthetic Roughtime draft08 test vector.\n");
  scr_printf("-------------------------------------------\n");
#endif
  rc=run_fixture();
#ifdef V12_PS2_RUNTIME
  if(rc==0) {
    scr_printf("ED25519 DELEGATION: PASS\n");
    scr_printf("ED25519 SIGNED RESPONSE: PASS\n");
    scr_printf("NONCE / MERKLE PROOF: PASS\n");
    scr_printf("WRONG ROOT/NONCE/TAMPER: REJECTED\n");
    scr_printf("N3B OFFLINE RESULT: PASS (code 0)\n");
  } else {
    scr_printf("N3B OFFLINE RESULT: FAIL (code %d)\n",rc);
  }
  scr_printf("NO TRUSTED LIVE TIME. NO HTTPS. NO DISK WRITES.\n");
  scr_printf("Screen shown for 90 sec; then returns to loader.\n");
  sleep(90);
#else
  printf("N3B OFFLINE RESULT: %s (code %d)\n",rc==0?"PASS":"FAIL",rc);
#endif
  return rc;
}

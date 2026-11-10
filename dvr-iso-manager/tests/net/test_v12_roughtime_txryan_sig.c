#include "v12_roughtime_txryan_sig.h"
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include "v12_rt_test_vector.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Host negative tests: the public fixture is signed by a DIFFERENT
 * synthetic root. Live TXRyan replies must never accept it. */
int main(void) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time tmp={0,0};
  uint8_t changed[sizeof(rt_fixture_reply)];
  assert(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
        rt_fixture_root,rt_fixture_nonce,&cb,&tmp)==0);
  assert(tmp.midpoint_utc==50 && tmp.radius_seconds==5);
  assert(v12_roughtime_txryan_sig_verify(rt_fixture_reply,
        sizeof(rt_fixture_reply),rt_fixture_nonce)<0);
  assert(v12_roughtime_txryan_sig_verify(
        rt_fixture_missing_nonc_reply,
        sizeof(rt_fixture_missing_nonc_reply),rt_fixture_nonce)<0);
  assert(v12_roughtime_txryan_sig_verify(NULL,1,rt_fixture_nonce)<0);
  assert(v12_roughtime_txryan_sig_verify(rt_fixture_reply,
        sizeof(rt_fixture_reply),NULL)<0);
  memcpy(changed,rt_fixture_reply,sizeof(changed));
  changed[64]^=0x01;
  assert(v12_roughtime_txryan_sig_verify(changed,
        sizeof(changed),rt_fixture_nonce)<0);
  puts("PASS: TXRyan pin rejects another server's signed fixture, missing-NONC fixture and corruption");
  return 0;
}

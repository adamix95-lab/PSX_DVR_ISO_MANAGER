#include "v12_roughtime_live_sig.h"
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include "v12_rt_test_vector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time stamp={0,0};
  uint8_t tamper[sizeof(rt_fixture_reply)];
  assert(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
        rt_fixture_root,rt_fixture_nonce,&cb,&stamp)==0);
  assert(stamp.midpoint_utc==50 && stamp.radius_seconds==5);
  /* Synthetic fixture MUST fail against the real Cloudflare root. */
  assert(v12_roughtime_live_sig_verify(rt_fixture_reply,
        sizeof(rt_fixture_reply),rt_fixture_nonce)<0);
  assert(v12_roughtime_live_sig_verify(NULL,1,rt_fixture_nonce)<0);
  assert(v12_roughtime_live_sig_verify(rt_fixture_reply,
        sizeof(rt_fixture_reply),NULL)<0);
  memcpy(tamper,rt_fixture_reply,sizeof(tamper));
  tamper[64]^=1;
  assert(v12_roughtime_live_sig_verify(tamper,
        sizeof(tamper),rt_fixture_nonce)<0);
  puts("PASS: N3B pin accepts only Cloudflare root and rejects synthetic/tampered vectors");
  return 0;
}

#include "v12_n3e_txryan.h"
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include "v12_rt_test_vector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* TXRyan pin must reject a fixture signed under another server key;
 * original reference verifier is separately required to accept it.
 */
int main(void) {
  v12_rt_crypto c=v12_rt_monocypher_backend();
  v12_rt_signed_time value={0,0},out={999,999};
  uint8_t copy[sizeof(rt_fixture_reply)];
  assert(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
      rt_fixture_root,rt_fixture_nonce,&c,&value)==0);
  assert(value.midpoint_utc==50 && value.radius_seconds==5);
  assert(v12_n3e_txryan_strict(rt_fixture_reply,sizeof(rt_fixture_reply),
                              rt_fixture_nonce,&out)<0);
  assert(out.midpoint_utc==999 && out.radius_seconds==999);
  assert(v12_n3e_txryan_strict(rt_fixture_missing_nonc_reply,
      sizeof(rt_fixture_missing_nonc_reply),rt_fixture_nonce,&out)<0);
  memcpy(copy,rt_fixture_reply,sizeof(copy));
  copy[50]^=1U;
  assert(v12_n3e_txryan_strict(copy,sizeof(copy),rt_fixture_nonce,&out)<0);
  assert(v12_n3e_txryan_strict(NULL,0,rt_fixture_nonce,&out)<0);
  assert(v12_n3e_txryan_strict(rt_fixture_reply,sizeof(rt_fixture_reply),
      NULL,&out)<0);
  assert(v12_n3e_txryan_strict(rt_fixture_reply,sizeof(rt_fixture_reply),
      rt_fixture_nonce,NULL)<0);
  puts("PASS: N3E rejects other keys, missing NONC, corrupt data and null input");
  return 0;
}

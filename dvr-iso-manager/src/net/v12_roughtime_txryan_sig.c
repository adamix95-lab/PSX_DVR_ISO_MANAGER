#include "v12_roughtime_txryan_sig.h"
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include <string.h>

/* PINNED PUBLIC ED25519 KEY for time.txryan.com:2002 from
 * cloudflare/roughtime/ecosystem.json (pin checked by GitHub Actions).
 * base64: iBVjxg/1j7y1+kQUTBYdTabxCppesU/07D4PMDJk2WA=
 * Never derive trust anchor from untrusted DNS/server payload.
 */
static const uint8_t txryan_root[32]={
  0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,
  0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,
  0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,
  0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60
};

int v12_roughtime_txryan_sig_verify(const uint8_t *packet,size_t len,
                                    const uint8_t nonce[32]) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time discarded={0,0};
  uint8_t wrong_root[32],wrong_nonce[32];
  int mode=0;
  if(!packet || !nonce) return -1;

  /* STRICT first: never accept an absent NONC as valid IETF draft08. */
  if(v12_rt_verify_draft08(packet,len,txryan_root,
                           nonce,&cb,&discarded)!=0) {
    /* Isolated forensic path may explain a noncompliant server but
     * may NEVER establish trusted time or certificate validity. */
    if(v12_rt_diagnostic_missing_nonce_echo(packet,len,txryan_root,
                                            nonce,&cb)!=0) return -2;
    mode=1;
  }
  memcpy(wrong_root,txryan_root,32);
  wrong_root[0]^=1;
  if((mode==0 && v12_rt_verify_draft08(packet,len,wrong_root,
                                       nonce,&cb,&discarded)==0) ||
     (mode==1 && v12_rt_diagnostic_missing_nonce_echo(packet,len,
                                         wrong_root,nonce,&cb)==0))
    return -3;

  memcpy(wrong_nonce,nonce,32);
  wrong_nonce[0]^=1;
  if((mode==0 && v12_rt_verify_draft08(packet,len,txryan_root,
                              wrong_nonce,&cb,&discarded)==0) ||
     (mode==1 && v12_rt_diagnostic_missing_nonce_echo(packet,len,
                              txryan_root,wrong_nonce,&cb)==0))
    return -4;

  /* No trust output; both modes are diagnostics with replayable nonce. */
  memset(&discarded,0,sizeof(discarded));
  return mode;
}

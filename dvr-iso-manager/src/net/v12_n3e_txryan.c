#include "v12_n3e_txryan.h"
#include "v12_roughtime_monocypher.h"
#include "v12_n3e_utc.h"
#include <string.h>

/* Pinned public TXRyan root from cloudflare/roughtime/ecosystem.json.
 * Independently compared to upstream and N3B pinned root in CI.
 * Do NOT copy the root from DNS or network replies.
 */
static const uint8_t pinned_root[32]={
  0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,
  0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,
  0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,
  0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60
};
int v12_n3e_txryan_strict(const uint8_t *packet,size_t length,
                           const uint8_t nonce[32],
                           v12_rt_signed_time *verified) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time local={0,0},discard={0,0};
  uint8_t wrong_key[32],wrong_nonce[32];
  char mid[20],lo[20],hi[20];
  if(!packet || !nonce || !verified) return -1;
  if(v12_rt_verify_draft08(packet,length,pinned_root,nonce,&cb,&local)!=0)
    return -2; /* STRICT draft08 only; no absent-NONC forensic fallback. */
  memcpy(wrong_key,pinned_root,sizeof(wrong_key));
  wrong_key[0]^=1U;
  if(v12_rt_verify_draft08(packet,length,wrong_key,nonce,&cb,&discard)==0)
    return -3;
  memcpy(wrong_nonce,nonce,sizeof(wrong_nonce));
  wrong_nonce[0]^=1U;
  if(v12_rt_verify_draft08(packet,length,pinned_root,wrong_nonce,&cb,&discard)==0)
    return -4;
  if(v12_n3e_signed_interval(&local,mid,lo,hi)!=0)
    return -5; /* Reject nonsensical time/radius; not proof of freshness. */
  *verified=local; /* Caller MUST keep this within diagnostic RAM. */
  return 0;
}

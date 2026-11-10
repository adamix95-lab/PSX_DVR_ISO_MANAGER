#include "v12_roughtime_live_sig.h"
#include "v12_roughtime_verify.h"
#include "v12_roughtime_monocypher.h"
#include <string.h>

/* Cloudflare-Roughtime-2 PUBLIC Ed25519 root from cloudflare/roughtime
 * ecosystem.json, base64 0GD7c3yP8xEc4Zl2zeuN2SlLvDVVocjsPSL8/Rl/7zg=
 * Rotated keys must be reviewed/pinned explicitly. Never trust a root
 * supplied by the network.
 */
static const uint8_t cloudflare_roughtime_root[32]={
  0xd0,0x60,0xfb,0x73,0x7c,0x8f,0xf3,0x11,
  0x1c,0xe1,0x99,0x76,0xcd,0xeb,0x8d,0xd9,
  0x29,0x4b,0xbc,0x35,0x55,0xa1,0xc8,0xec,
  0x3d,0x22,0xfc,0xfd,0x19,0x7f,0xef,0x38
};
int v12_roughtime_live_sig_verify(const uint8_t *reply,size_t n,
                                  const uint8_t nonce[32]) {
  v12_rt_crypto cb=v12_rt_monocypher_backend();
  v12_rt_signed_time verified={0,0};
  uint8_t bad_root[32],bad_nonce[32];
  if(!reply || !nonce) return -1;
  if(v12_rt_verify_draft08(reply,n,cloudflare_roughtime_root,
                           nonce,&cb,&verified)!=0) return -2;
  memcpy(bad_root,cloudflare_roughtime_root,32);
  bad_root[0]^=1;
  if(v12_rt_verify_draft08(reply,n,bad_root,nonce,&cb,&verified)==0)
    return -3;
  memcpy(bad_nonce,nonce,32);
  bad_nonce[0]^=1;
  if(v12_rt_verify_draft08(reply,n,cloudflare_roughtime_root,
                           bad_nonce,&cb,&verified)==0) return -4;
  /* No time output is exposed. In particular, not to wolfSSL or RTC. */
  memset(&verified,0,sizeof(verified));
  return 0;
}

#include "v12_rt_reservation.h"
#include <string.h>

static int matching(const v12_rt_reservation *x,
                    const v12_rt_reservation *y) {
  return x->last_reserved == y->last_reserved &&
         memcmp(x->key_id,y->key_id,sizeof(x->key_id)) == 0;
}

int v12_rt_reserve_nonce(const uint8_t secret[32],
                         const uint8_t expected_key_id[16],
                         const v12_rt_reservation_store *store,
                         v12_rt_hmac256 hmac, void *hmac_ctx,
                         uint8_t nonce[32]) {
  v12_rt_reservation old, next, confirmed;
  uint8_t candidate[32];
  unsigned nonzero=0,i;
  if(!secret || !expected_key_id || !nonce || !store ||
     !store->load || !store->reserve_and_sync || !hmac)
    return -1;
  for(i=0;i<32;i++) nonzero|=secret[i];
  if(!nonzero) return -1;
  nonzero=0;
  for(i=0;i<16;i++) nonzero|=expected_key_id[i];
  if(!nonzero) return -1;

  memset(&old,0,sizeof(old));
  if(store->load(store->ctx,&old)!=0 ||
     memcmp(old.key_id,expected_key_id,16) ||
     old.last_reserved>=UINT64_MAX-1U)
    return -1;

  next=old;
  ++next.last_reserved;
  /* The durable CAS must finish BEFORE any nonce becomes available. */
  if(store->reserve_and_sync(store->ctx,&old,&next)!=0)
    return -1;
  memset(&confirmed,0,sizeof(confirmed));
  if(store->load(store->ctx,&confirmed)!=0 ||
     !matching(&confirmed,&next))
    return -1;

  /* A failed derivation burns the reserved counter. This is safe and
   * preferable to emitting a nonce without durable state. */
  if(v12_rt_nonce_derive(secret,next.last_reserved,hmac,hmac_ctx,candidate)!=0)
    return -1;
  memcpy(nonce,candidate,sizeof(candidate));
  memset(candidate,0,sizeof(candidate));
  return 0;
}

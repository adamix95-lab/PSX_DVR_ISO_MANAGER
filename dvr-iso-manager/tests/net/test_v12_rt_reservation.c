#include "v12_rt_reservation.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* A test double, NOT a real PFS durable journal. */
typedef struct {
  v12_rt_reservation state;
  int reads,commits,load_error,cas_error,readback_stale;
} mock_store;

static int load(void *p,v12_rt_reservation *out) {
  mock_store *m=(mock_store*)p;
  ++m->reads;
  if(m->load_error) return -1;
  *out=m->state;
  if(m->readback_stale && m->commits) --out->last_reserved;
  return 0;
}
static int reserve(void *p,const v12_rt_reservation *old,
                   const v12_rt_reservation *next) {
  mock_store *m=(mock_store*)p;
  ++m->commits;
  if(m->cas_error ||
     memcmp(old->key_id,m->state.key_id,16) ||
     old->last_reserved!=m->state.last_reserved ||
     next->last_reserved!=old->last_reserved+1U ||
     memcmp(next->key_id,old->key_id,16)) return -1;
  m->state=*next;
  return 0; /* Mock assumes durability. Actual HDD needs separate testing. */
}
static int mac(void *ctx,const uint8_t key[32],const uint8_t *data,
               size_t size,uint8_t digest[32]) {
  unsigned n=0;
  (void)ctx;
  return HMAC(EVP_sha256(),key,32,data,size,digest,&n)!=NULL && n==32 ?
         0:-1;
}
static int fail_mac(void *ctx,const uint8_t key[32],const uint8_t *data,
                    size_t size,uint8_t digest[32]) {
  (void)ctx;(void)key;(void)data;(void)size;(void)digest;
  return -1;
}
static void setup(mock_store *m,v12_rt_reservation_store *cb,
                  uint8_t key_id[16]) {
  memset(m,0,sizeof(*m));
  for(unsigned i=0;i<16;i++) key_id[i]=(uint8_t)(0x40+i);
  memcpy(m->state.key_id,key_id,16);
  m->state.last_reserved=41;
  cb->ctx=m;cb->load=load;cb->reserve_and_sync=reserve;
}
static void guard(const uint8_t secret[32],const uint8_t id[16],
                  v12_rt_reservation_store *cb,mock_store *m) {
  uint8_t nonce[32];
  memset(nonce,0x5a,sizeof(nonce));
  assert(v12_rt_reserve_nonce(secret,id,cb,mac,NULL,nonce)<0);
  for(unsigned i=0;i<32;i++) assert(nonce[i]==0x5a);
  (void)m;
}
int main(void) {
  uint8_t secret[32],id[16],first[32],second[32],expected[32],wrong_id[16];
  mock_store m;
  v12_rt_reservation_store cb;
  for(unsigned i=0;i<32;i++) secret[i]=(uint8_t)(i+1U);
  setup(&m,&cb,id);

  assert(v12_rt_reserve_nonce(secret,id,&cb,mac,NULL,first)==0);
  assert(m.state.last_reserved==42 && m.commits==1 && m.reads==2);
  assert(v12_rt_nonce_derive(secret,42,mac,NULL,expected)==0);
  assert(!memcmp(first,expected,32));
  assert(v12_rt_reserve_nonce(secret,id,&cb,mac,NULL,second)==0);
  assert(m.state.last_reserved==43 && m.commits==2 && m.reads==4);
  assert(memcmp(first,second,32)!=0);

  /* Missing/invalid persisted identity: NEVER silently reinitialize. */
  setup(&m,&cb,id);
  memcpy(wrong_id,id,16);
  wrong_id[2]^=1;
  guard(secret,wrong_id,&cb,&m);
  assert(m.commits==0);

  /* Storage errors must be fail-closed, with untouched nonce buffer. */
  setup(&m,&cb,id);
  m.load_error=1;
  guard(secret,id,&cb,&m);
  assert(m.commits==0);
  setup(&m,&cb,id);
  m.cas_error=1;
  guard(secret,id,&cb,&m);
  assert(m.commits==1 && m.state.last_reserved==41);

  /* Even a claimed durable commit must be RE-READ and checked. */
  setup(&m,&cb,id);
  m.readback_stale=1;
  guard(secret,id,&cb,&m);
  assert(m.commits==1 && m.state.last_reserved==42);

  /* A KDF failure burns the reserved counter; never attempt rollback. */
  setup(&m,&cb,id);
  memset(expected,0xa5,sizeof(expected));
  assert(v12_rt_reserve_nonce(secret,id,&cb,fail_mac,NULL,expected)<0);
  assert(m.state.last_reserved==42);
  for(unsigned i=0;i<32;i++) assert(expected[i]==0xa5);

  setup(&m,&cb,id);
  m.state.last_reserved=UINT64_MAX-1U;
  guard(secret,id,&cb,&m);
  assert(m.commits==0);

  /* Rollback of both seed and state would REPEAT a nonce. This cannot be
   * solved in the helper; backups/restores require a NEW secret + key ID.
   */
  setup(&m,&cb,id);
  assert(v12_rt_reserve_nonce(secret,id,&cb,mac,NULL,first)==0);
  setup(&m,&cb,id); /* Simulated snapshot restoration: previous counter. */
  assert(v12_rt_reserve_nonce(secret,id,&cb,mac,NULL,second)==0);
  assert(memcmp(first,second,32)==0); /* Demonstrates replay vulnerability. */
  /* Re-provisioning with a new CSPRNG seed is mandatory after rollback. */
  secret[0]^=0x55;
  setup(&m,&cb,id);
  id[0]^=0x12;
  memcpy(m.state.key_id,id,16);
  assert(v12_rt_reserve_nonce(secret,id,&cb,mac,NULL,second)==0);
  assert(memcmp(first,second,32)!=0);

  puts("PASS: N3C reservation gate, durable-CAS mock, readback, errors, rollback/rekey policy");
  return 0;
}

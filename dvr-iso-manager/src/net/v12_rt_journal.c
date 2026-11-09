#include "v12_rt_journal.h"
#include <string.h>

static const uint8_t magic[8]={'P','D','I','M','R','T','1','2'};
static const uint8_t tail[4]={'R','T','1','2'};

static void put32(uint8_t *p,uint32_t n) {
  unsigned i;
  for(i=0;i<4;i++) p[i]=(uint8_t)(n>>(i*8U));
}
static void put64(uint8_t *p,uint64_t n) {
  unsigned i;
  for(i=0;i<8;i++) p[i]=(uint8_t)(n>>(i*8U));
}
static uint32_t get32(const uint8_t *p) {
  uint32_t x=0;
  unsigned i;
  for(i=0;i<4;i++) x|=(uint32_t)p[i]<<(i*8U);
  return x;
}
static uint64_t get64(const uint8_t *p) {
  uint64_t x=0;
  unsigned i;
  for(i=0;i<8;i++) x|=(uint64_t)p[i]<<(i*8U);
  return x;
}
static uint32_t crc32(const uint8_t *p,size_t n) {
  uint32_t c=UINT32_C(0xffffffff);
  size_t i;
  for(i=0;i<n;i++) {
    unsigned j;
    c^=p[i];
    for(j=0;j<8;j++) {
      c=(c>>1)^((c&1U)?UINT32_C(0xedb88320):0U);
    }
  }
  return c^UINT32_C(0xffffffff);
}
static int id_nonzero(const uint8_t *id) {
  unsigned i,nonzero=0;
  for(i=0;i<16;i++) nonzero|=id[i];
  return nonzero!=0;
}
static int blank(const uint8_t *p) {
  unsigned i;
  for(i=0;i<V12_RT_RECORD_BYTES;i++) if(p[i]!=0) return 0;
  return 1;
}

int v12_rt_journal_encode(const v12_rt_journal_state *s,
                          uint8_t out[V12_RT_RECORD_BYTES]) {
  uint8_t rec[V12_RT_RECORD_BYTES];
  if(!s || !out || !id_nonzero(s->reservation.key_id) ||
     !s->sequence || s->sequence==UINT64_MAX ||
     s->reservation.last_reserved>=UINT64_MAX-1U)
    return -1;
  memset(rec,0,sizeof(rec));
  memcpy(rec,magic,sizeof(magic));
  put32(rec+8,1U);
  /* rec[12..15] must stay zero (reserved for future versioning). */
  memcpy(rec+16,s->reservation.key_id,16);
  put64(rec+32,s->sequence);
  put64(rec+40,s->reservation.last_reserved);
  put64(rec+48,~s->reservation.last_reserved);
  put32(rec+56,crc32(rec,56));
  memcpy(rec+60,tail,4);
  memcpy(out,rec,sizeof(rec));
  return 0;
}
int v12_rt_journal_decode(const uint8_t record[V12_RT_RECORD_BYTES],
                          v12_rt_journal_state *out) {
  v12_rt_journal_state state;
  if(!record || !out || memcmp(record,magic,8) ||
     get32(record+8)!=1U || get32(record+12)!=0U ||
     memcmp(record+60,tail,4) ||
     get32(record+56)!=crc32(record,56) ||
     !id_nonzero(record+16))
    return -1;
  memset(&state,0,sizeof(state));
  memcpy(state.reservation.key_id,record+16,16);
  state.sequence=get64(record+32);
  state.reservation.last_reserved=get64(record+40);
  if(state.sequence==0 || state.sequence==UINT64_MAX ||
     state.reservation.last_reserved>=UINT64_MAX-1U ||
     get64(record+48)!=~state.reservation.last_reserved)
    return -1;
  *out=state;
  return 0;
}
int v12_rt_journal_select(const uint8_t slots[2][V12_RT_RECORD_BYTES],
                          v12_rt_journal_state *out,unsigned *active) {
  v12_rt_journal_state a,b,chosen;
  int ok0,ok1,b0,b1;
  unsigned which;
  if(!slots || !out || !active) return -1;
  ok0=v12_rt_journal_decode(slots[0],&a)==0;
  ok1=v12_rt_journal_decode(slots[1],&b)==0;
  b0=blank(slots[0]); b1=blank(slots[1]);
  /* A damaged nonempty slot is NEVER treated as if it did not exist. */
  if((!ok0 && !b0) || (!ok1 && !b1) || (!ok0 && !ok1))
    return -1;
  if(ok0 && ok1) {
    if(memcmp(a.reservation.key_id,b.reservation.key_id,16))
      return -1;
    if(a.sequence==b.sequence) {
      if(memcmp(slots[0],slots[1],V12_RT_RECORD_BYTES)) return -1;
    } else {
      /* Alternating slots must be consecutive in BOTH generation and
       * reserved counter; a fabricated 'newer' but smaller counter is
       * a rollback, not a valid recovery. Fail closed. */
      const v12_rt_journal_state *hi = a.sequence>b.sequence?&a:&b;
      const v12_rt_journal_state *lo = a.sequence>b.sequence?&b:&a;
      if(lo->sequence+1U!=hi->sequence ||
         lo->reservation.last_reserved+1U!=
           hi->reservation.last_reserved)
        return -1;
    }
    which=a.sequence>=b.sequence?0U:1U;
    chosen=which==0?a:b;
  } else {
    which=ok0?0U:1U;
    chosen=ok0?a:b;
    /* A blank slot is legal ONLY at initial provisioning. */
    if(chosen.sequence!=1U) return -1;
  }
  *out=chosen;
  *active=which;
  return 0;
}

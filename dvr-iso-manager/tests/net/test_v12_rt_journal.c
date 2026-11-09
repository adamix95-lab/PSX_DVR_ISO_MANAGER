#include "v12_rt_journal.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void unchanged(const uint8_t record[64]) {
  v12_rt_journal_state out={0};
  out.sequence=UINT64_C(888);
  out.reservation.last_reserved=UINT64_C(555);
  assert(v12_rt_journal_decode(record,&out)<0);
  assert(out.sequence==UINT64_C(888));
  assert(out.reservation.last_reserved==UINT64_C(555));
}
int main(void) {
  v12_rt_journal_state state={0},result={0};
  uint8_t rec[64],bad[64],slots[2][64]={{0}},different[64];
  unsigned active=99;
  for(unsigned i=0;i<16;i++) state.reservation.key_id[i]=(uint8_t)(i+1U);
  state.sequence=1U;state.reservation.last_reserved=41U;
  assert(v12_rt_journal_encode(&state,rec)==0);
  assert(!memcmp(rec,"PDIMRT12",8));
  assert(v12_rt_journal_decode(rec,&result)==0);
  assert(result.sequence==1U && result.reservation.last_reserved==41U);
  assert(!memcmp(result.reservation.key_id,state.reservation.key_id,16));

  /* A single flipped bit anywhere in the record is always detected. */
  for(unsigned i=0;i<sizeof(rec);i++)
    for(unsigned bit=0;bit<8;bit++) {
      memcpy(bad,rec,sizeof(bad));
      bad[i]^=(uint8_t)(1U<<bit);
      unchanged(bad);
    }
  unchanged(slots[0]); /* Empty file must not initialize a new counter. */
  memcpy(slots[0],rec,sizeof(rec));
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)==0);
  assert(active==0 && result.sequence==1);

  /* Correct alternating writes select the strictly newer sequence. */
  state.sequence=2U;state.reservation.last_reserved=42U;
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)==0);
  assert(active==1 && result.reservation.last_reserved==42U);
  state.sequence=3U;state.reservation.last_reserved=43U;
  assert(v12_rt_journal_encode(&state,slots[0])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)==0);
  assert(active==0 && result.reservation.last_reserved==43U);

  /* No automatic fallback if EITHER active or old slot is corrupted. */
  slots[1][30]^=1;
  active=99;
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);
  assert(active==99);
  slots[1][30]^=1;
  slots[0][15]^=1;
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);
  slots[0][15]^=1;

  /* Matching generation with different payload is ambiguous: reject. */
  memcpy(different,slots[0],64);
  state.sequence=3U;state.reservation.last_reserved=44U;
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);
  memcpy(slots[0],different,64);

  /* Consecutive generation MUST also reserve a consecutive nonce.
   * A higher-generation/lower-counter replacement would repeat a nonce. */
  state.sequence=4U;
  state.reservation.last_reserved=40U; /* Rollback below the 43 in SLOT0 */
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);
  state.reservation.last_reserved=46U; /* Skipped reservations */
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);
  state.sequence=6U; /* Skipped journal generation */
  state.reservation.last_reserved=44U;
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);

  /* Different seed identity is a hard failure, even with later sequence. */
  state.sequence=4U;
  state.reservation.key_id[0]^=0x44;
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);

  /* Only initial seq=1 can coexist with a blank slot. */
  memset(slots,0,sizeof(slots));
  state.sequence=2U;
  assert(v12_rt_journal_encode(&state,slots[1])==0);
  assert(v12_rt_journal_select((const uint8_t (*)[64])slots,&result,&active)<0);

  /* Invalid counters/sequences do not modify the intended output bytes. */
  memset(bad,0xA5,sizeof(bad));
  state.sequence=UINT64_MAX;
  assert(v12_rt_journal_encode(&state,bad)<0);
  for(unsigned i=0;i<64;i++) assert(bad[i]==0xA5);
  state.sequence=1U;state.reservation.last_reserved=UINT64_MAX-1U;
  assert(v12_rt_journal_encode(&state,bad)<0);
  for(unsigned i=0;i<64;i++) assert(bad[i]==0xA5);

  puts("PASS: N3C journal version/CRC, 512 bit flips, two-slot selection and fail-closed corruption");
  return 0;
}

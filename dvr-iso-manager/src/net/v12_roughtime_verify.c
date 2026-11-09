#include "v12_roughtime_verify.h"
#include "v12_roughtime_wire.h"
#include <string.h>

/* Independent bounded parser for the IETF draft-08 map encoding.
 * Root/certificate fields are untrusted until BOTH signatures + Merkle
 * inclusion verify. No unbounded allocations, pointers, I/O or callbacks.
 */
typedef struct { const uint8_t *p; size_t n; } rt_span;
typedef struct { rt_span key[32], value[32]; unsigned n; } rt_map;
static uint32_t u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
         ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static uint64_t u64(const uint8_t *p) {
  return (uint64_t)u32(p) | ((uint64_t)u32(p+4)<<32);
}
static int parse(rt_span src,rt_map *m) {
  uint32_t n,prev=0,off=0,i;
  size_t base,valsz;
  if(!src.p || !m || src.n<12 || (src.n&3U)) return -1;
  n=u32(src.p);
  if(n<1 || n>32 || (size_t)n>src.n/8U) return -1;
  base=(size_t)n*8U;
  if(base>src.n) return -1;
  valsz=src.n-base;
  memset(m,0,sizeof(*m));
  for(i=0;i<n;++i) {
    uint32_t tag=u32(src.p+(size_t)n*4U+(size_t)i*4U);
    uint32_t next=(i+1<n)?u32(src.p+4U+(size_t)i*4U):
                            (uint32_t)valsz;
    if((i && tag<=prev) || next<off || (next&3U) || next>valsz)
      return -1;
    m->key[i].p=src.p+(size_t)n*4U+(size_t)i*4U;
    m->key[i].n=4;
    m->value[i].p=src.p+base+off;
    m->value[i].n=(size_t)(next-off);
    prev=tag;off=next;
  }
  m->n=n;
  return 0;
}
static int get(const rt_map *m,const char tag[4],rt_span *v) {
  unsigned i;
  if(!m||!v) return -1;
  for(i=0;i<m->n;i++)
    if(!memcmp(m->key[i].p,tag,4)) {
      *v=m->value[i];return 0;
    }
  return -1;
}
static int fixed(const rt_map *m,const char key[4],size_t expected,rt_span *v) {
  return get(m,key,v)==0 && v->n==expected?0:-1;
}
static int signed_verify(const v12_rt_crypto *cb,const uint8_t key[32],
                         rt_span signature,const char *context,size_t context_size,
                         rt_span content) {
  uint8_t input[V12_RT_REPLY_MAX_BYTES+64];
  if(signature.n!=64 || context_size>64 || content.n>
     sizeof(input)-context_size) return -1;
  memcpy(input,context,context_size);
  memcpy(input+context_size,content.p,content.n);
  return cb->verify_ed25519(cb->ctx,key,signature.p,input,
                             context_size+content.n);
}
static int make_digest(const v12_rt_crypto *cb,const uint8_t *data,
                       size_t n,uint8_t output[32]) {
  uint8_t full[64];
  if(cb->sha512(cb->ctx,data,n,full)!=0) return -1;
  memcpy(output,full,32);
  return 0;
}

int v12_rt_verify_draft08(const uint8_t *reply,size_t reply_size,
                          const uint8_t rootkey[32],
                          const uint8_t nonce[32],
                          const v12_rt_crypto *cb,
                          v12_rt_signed_time *output) {
  static const char cert_ctx[]="RoughTime v1 delegation signature--";
  static const char rep_ctx[]="RoughTime v1 response signature";
  rt_map top,cert,delegation,signedrep;
  rt_span s,ver,nc,certbytes,dele,sig_dele,onlinekey,srep,sig_rep;
  rt_span mint,maxt,midp,radi,merkleroot,index,path;
  uint8_t buf[65],current[32],next[32];
  uint32_t path_index,uncertainty;
  uint64_t start,stop,midpoint;
  size_t offset;
  v12_rt_signed_time parsed;
  int digest_rc;
  if(!reply || !rootkey || !nonce || !cb || !output ||
     !cb->sha512 || !cb->verify_ed25519 ||
     reply_size<32 || reply_size>V12_RT_REPLY_MAX_BYTES ||
     memcmp(reply,"ROUGHTIM",8) || (reply_size&3U) ||
     u32(reply+8)!=reply_size-12U) return -1;
  s.p=reply+12;s.n=reply_size-12U;
  if(parse(s,&top)<0) return -1;
  if(fixed(&top,"VER\0",4,&ver)<0 || u32(ver.p)!=UINT32_C(0x80000008) ||
     fixed(&top,"NONC",32,&nc)<0 || memcmp(nc.p,nonce,32))
    return -1;

  if(get(&top,"CERT",&certbytes)<0 || parse(certbytes,&cert)<0 ||
     get(&cert,"DELE",&dele)<0 ||
     fixed(&cert,"SIG\0",64,&sig_dele)<0 ||
     parse(dele,&delegation)<0 ||
     fixed(&delegation,"PUBK",32,&onlinekey)<0 ||
     fixed(&delegation,"MINT",8,&mint)<0 ||
     fixed(&delegation,"MAXT",8,&maxt)<0) return -1;

  /* Signed delegation must authenticate the online key before using it. */
  if(signed_verify(cb,rootkey,sig_dele,cert_ctx,sizeof(cert_ctx),dele)!=0)
    return -1;
  start=u64(mint.p);stop=u64(maxt.p);
  if(stop<start) return -1;
  if(get(&top,"SREP",&srep)<0 ||
     fixed(&top,"SIG\0",64,&sig_rep)<0 ||
     parse(srep,&signedrep)<0 ||
     fixed(&signedrep,"MIDP",8,&midp)<0 ||
     fixed(&signedrep,"RADI",4,&radi)<0 ||
     fixed(&signedrep,"ROOT",32,&merkleroot)<0) return -1;
  if(signed_verify(cb,onlinekey.p,sig_rep,rep_ctx,sizeof(rep_ctx),srep)!=0)
    return -1;
  midpoint=u64(midp.p);uncertainty=u32(radi.p);
  if(midpoint<start || midpoint>stop || uncertainty>86400U) return -1;
  if(fixed(&top,"INDX",4,&index)<0 || get(&top,"PATH",&path)<0 ||
     (path.n%32U) || path.n>32U*24U) return -1;
  path_index=u32(index.p);

  /* Roughtime Merkle leaf: SHA512(0x00 || nonce), first 32 bytes. */
  buf[0]=0;
  memcpy(buf+1,nonce,32);
  digest_rc=make_digest(cb,buf,33,current);
  if(digest_rc<0) return -1;
  for(offset=0;offset<path.n;offset+=32) {
    buf[0]=1;
    if(path_index&1U) {
      memcpy(buf+1,path.p+offset,32);
      memcpy(buf+33,current,32);
    } else {
      memcpy(buf+1,current,32);
      memcpy(buf+33,path.p+offset,32);
    }
    if(make_digest(cb,buf,65,next)<0) return -1;
    memcpy(current,next,32);
    path_index>>=1;
  }
  if(path_index || memcmp(current,merkleroot.p,32)) return -1;
  parsed.midpoint_utc=midpoint;
  parsed.radius_seconds=uncertainty;
  *output=parsed;
  return 0;
}

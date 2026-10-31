#include "probe_png.h"
#include <string.h>

/* No allocations, network APIs, filesystem APIs or external dependencies.
   Reject incomplete, oversized, malformed or CRC-corrupt PNG streams. */
static uint32_t be32(const uint8_t *p) {
  return ((uint32_t)p[0]<<24) | ((uint32_t)p[1]<<16) |
         ((uint32_t)p[2]<<8) | (uint32_t)p[3];
}
static uint32_t crc_step(uint32_t crc, uint8_t byte) {
  unsigned i;
  crc ^= (uint32_t)byte;
  for (i = 0; i < 8; i++)
    crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
  return crc;
}
int probe_png_check(const uint8_t *data, size_t length) {
  static const uint8_t signature[8] = {137,80,78,71,13,10,26,10};
  size_t offset=8;
  int ihdr=0, idat=0;
  if (!data || length < 57 || length > PROBE_PNG_MAX ||
      memcmp(data,signature,8)) return -1;
  while (offset < length) {
    uint32_t size, crc=0xFFFFFFFFU, expected;
    const uint8_t *type, *payload;
    size_t i;
    if (length-offset < 12U) return -1;
    size=be32(data+offset);
    if (size > length-offset-12U) return -1;
    type=data+offset+4;
    payload=data+offset+8;
    for (i=0;i<4;i++) {
      if (!((type[i]>='A'&&type[i]<='Z') ||
            (type[i]>='a'&&type[i]<='z'))) return -1;
    }
    for (i=0;i<(size_t)size+4U;i++)
      crc=crc_step(crc,type[i]);
    expected=be32(payload+size);
    if ((crc^0xFFFFFFFFU)!=expected) return -1;
    if (!memcmp(type,"IHDR",4)) {
      uint32_t width, height;
      if (ihdr || offset!=8 || size!=13) return -1;
      width=be32(payload);
      height=be32(payload+4);
      if (width<1 || width>2048 || height<1 || height>2048 ||
          payload[8]!=8 ||
          !(payload[9]==2 || payload[9]==3 || payload[9]==6) ||
          payload[10]!=0 || payload[11]!=0 || payload[12]!=0)
        return -1;
      ihdr=1;
    } else if (!memcmp(type,"IDAT",4)) {
      if (!ihdr || size==0) return -1;
      idat=1;
    } else if (!memcmp(type,"IEND",4)) {
      if (!ihdr || !idat || size!=0 ||
          offset + 12U != length) return -1;
      return 0;
    } else if (!ihdr) return -1;
    offset += 12U + (size_t)size;
  }
  return -1;
}
static int case_prefix(const uint8_t *p, size_t n, const char *word) {
  size_t i, wlen=strlen(word);
  if (wlen>n) return 0;
  for (i=0;i<wlen;i++) {
    char c=(char)p[i], w=word[i];
    if (c>='A' && c<='Z') c=(char)(c+32);
    if (w>='A' && w<='Z') w=(char)(w+32);
    if (c!=w) return 0;
  }
  return 1;
}
int probe_http_extract_png(const uint8_t *data, size_t length,
                           const uint8_t **png, size_t *png_length) {
  size_t header_end=0, line=0, body_size=0;
  int got_size=0, got_type=0;
  if (!data || !png || !png_length ||
      length < 17 || length > PROBE_HTTP_HEADER_MAX + PROBE_PNG_MAX)
    return -1;
  *png=NULL;
  *png_length=0;
  if (memcmp(data,"HTTP/1.",7) ||
      (data[7]!='0' && data[7]!='1') ||
      memcmp(data+8," 200 ",5)) return -1;
  for (header_end=13;header_end+3<length &&
       header_end<PROBE_HTTP_HEADER_MAX;header_end++)
    if (!memcmp(data+header_end,"\r\n\r\n",4)) break;
  if (header_end+3>=length || header_end>=PROBE_HTTP_HEADER_MAX)
    return -1;
  /* The first header line is the HTTP status. */
  while (line+1<header_end &&
         (data[line]!='\r'||data[line+1]!='\n')) line++;
  if (line+1>=header_end) return -1;
  line+=2;
  while (line<header_end) {
    size_t end=line, pos;
    while (end+1<header_end &&
           !(data[end]=='\r'&&data[end+1]=='\n')) end++;
    /* Final header line ends at header_end (the first CRLF of CRLFCRLF). */
    if (end+1>=header_end) end=header_end;
    if (case_prefix(data+line,end-line,"Content-Length:")) {
      size_t n=0;
      if (got_size++) return -1;
      pos=line+15;
      while (pos<end && data[pos]==' ') pos++;
      if (pos==end) return -1;
      for (;pos<end;pos++) {
        if (data[pos]<'0'||data[pos]>'9') return -1;
        n=n*10U+(unsigned)(data[pos]-'0');
        if (n>PROBE_PNG_MAX) return -1;
      }
      body_size=n;
    } else if (case_prefix(data+line,end-line,"Content-Type:")) {
      if (got_type++) return -1;
      pos=line+13;
      while (pos<end && data[pos]==' ') pos++;
      if (end-pos!=9 || !case_prefix(data+pos,9,"image/png"))
        return -1;
    } else if (case_prefix(data+line,end-line,"Transfer-Encoding:")) {
      return -1; /* No chunked/streaming encoding support. */
    }
    line=end+2;
  }
  if (!got_size || !got_type || body_size<57 ||
      length-(header_end+4U)!=body_size ||
      probe_png_check(data+header_end+4U,body_size)<0) return -1;
  *png=data+header_end+4U;
  *png_length=body_size;
  return 0;
}

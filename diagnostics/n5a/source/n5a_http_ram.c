#include "n5a_http_ram.h"
#include <string.h>

static int eq_ci(unsigned char a,unsigned char b) {
    if(a>='A'&&a<='Z')a=(unsigned char)(a+('a'-'A'));
    if(b>='A'&&b<='Z')b=(unsigned char)(b+('a'-'A'));
    return a==b;
}
static int equals_ci(const uint8_t *p,size_t n,const char *word) {
    size_t i=0,m=strlen(word);
    if(n!=m)return 0;
    for(;i<n;i++)if(!eq_ci(p[i],(unsigned char)word[i]))return 0;
    return 1;
}
static size_t crlf_at(const uint8_t *s,size_t lo,size_t len) {
    for(size_t i=lo;i+1<len;i++)if(s[i]=='\r'&&s[i+1]=='\n')return i;
    return len;
}
static int only_body(const uint8_t *b,size_t n) {
    return n==N5A_HTTP_EXPECTED_BODY_BYTES &&
           memcmp(b,N5A_HTTP_EXPECTED_BODY,n)==0;
}
int n5a_http_check(const uint8_t *data,size_t len,int eof,n5a_http_result *out) {
    size_t head_end=0,pos,body_start,expected=0,body_size=0;
    int has_len=0,is_chunked=0,has_te=0,has_encoding=0;
    n5a_http_result r={0,0,0,0};
    char body[N5A_HTTP_BODY_CAP];
    if(!data || !out || len>N5A_HTTP_INPUT_CAP || (eof!=0&&eof!=1))
        return -1;
    if(len<16U)return eof?-2:0;
    for(size_t i=0;i+3<len && i<N5A_HTTP_HEADER_CAP;i++)
        if(data[i]=='\r'&&data[i+1]=='\n'&&
           data[i+2]=='\r'&&data[i+3]=='\n') {head_end=i+4;break;}
    if(!head_end) {
        if(len>=N5A_HTTP_HEADER_CAP || eof)return -2;
        return 0;
    }
    pos=crlf_at(data,0,head_end);
    if(pos>=head_end)return -3;
    /* Exact HTTP version and successful status; redirects are forbidden. */
    if(!((pos>=13U && memcmp(data,"HTTP/1.1 200 ",13)==0) ||
         (pos>=13U && memcmp(data,"HTTP/1.0 200 ",13)==0)))
        return -4;
    r.status=200;
    pos+=2;
    while(pos<head_end-2) {
        size_t end=crlf_at(data,pos,head_end),colon=pos,name_end;
        size_t v,ve;
        if(end>=head_end)return -5;
        if(end==pos)break;
        for(;colon<end && data[colon]!=':';colon++) {}
        if(colon==end || colon==pos)return -5;
        name_end=colon;
        while(name_end>pos && (data[name_end-1]==' '||data[name_end-1]=='\t'))
            --name_end;
        v=colon+1;ve=end;
        while(v<ve && (data[v]==' '||data[v]=='\t'))v++;
        while(ve>v && (data[ve-1]==' '||data[ve-1]=='\t'))ve--;
        if(equals_ci(data+pos,name_end-pos,"Content-Length")) {
            size_t number=0;
            if(has_len || v==ve)return -6;
            has_len=1;
            for(size_t j=v;j<ve;j++) {
                if(data[j]<'0'||data[j]>'9')return -6;
                number=number*10U+(size_t)(data[j]-'0');
                if(number>N5A_HTTP_BODY_CAP)return -7;
            }
            expected=number;
        }else if(equals_ci(data+pos,name_end-pos,"Transfer-Encoding")) {
            if(has_te)return -8;
            has_te=1;
            if(!equals_ci(data+v,ve-v,"chunked"))return -8;
            is_chunked=1;
        }else if(equals_ci(data+pos,name_end-pos,"Content-Encoding")) {
            if(has_encoding)return -9;
            has_encoding=1;
            if(!equals_ci(data+v,ve-v,"identity"))return -9;
        }
        pos=end+2;
    }
    if(is_chunked&&has_len)return -10;
    body_start=head_end;
    if(is_chunked) {
        pos=body_start;
        for(unsigned chunks=0;chunks<24U;chunks++) {
            size_t line_end=crlf_at(data,pos,len),chunk_len=0;
            if(line_end>=len)return eof?-11:0;
            if(line_end==pos || line_end-pos>24U)return -11;
            for(size_t j=pos;j<line_end;j++) {
                unsigned dig;
                if(data[j]>='0'&&data[j]<='9')dig=(unsigned)(data[j]-'0');
                else if(data[j]>='a'&&data[j]<='f')
                    dig=(unsigned)(data[j]-'a'+10);
                else if(data[j]>='A'&&data[j]<='F')
                    dig=(unsigned)(data[j]-'A'+10);
                else return -11;  /* no chunk extensions */
                chunk_len=chunk_len*16U+dig;
                if(chunk_len>N5A_HTTP_BODY_CAP-body_size)return -7;
            }
            pos=line_end+2;
            if(chunk_len==0U) {
                if(len-pos<2U)return eof?-11:0;
                if(data[pos]!='\r'||data[pos+1]!='\n')return -11;
                if(!only_body((const uint8_t *)body,body_size))return -12;
                r.header_bytes=(unsigned)head_end;
                r.body_bytes=(unsigned)body_size;
                r.chunked=1;
                *out=r;
                return 1;
            }
            if(len-pos<chunk_len+2U)return eof?-11:0;
            memcpy(body+body_size,data+pos,chunk_len);
            body_size+=chunk_len;
            pos+=chunk_len;
            if(data[pos]!='\r'||data[pos+1]!='\n')return -11;
            pos+=2;
        }
        return -11;
    }
    if(has_len) {
        if(expected!=N5A_HTTP_EXPECTED_BODY_BYTES)return -12;
        if(len-body_start<expected)return eof?-13:0;
        if(!only_body(data+body_start,expected))return -12;
        r.header_bytes=(unsigned)head_end;
        r.body_bytes=(unsigned)expected;
        *out=r;
        return 1;
    }
    /* With neither CL nor chunked, only EOF can delimit the body. */
    if(len-body_start>N5A_HTTP_BODY_CAP)return -7;
    if(!eof)return 0;
    if(!only_body(data+body_start,len-body_start))return -12;
    r.header_bytes=(unsigned)head_end;
    r.body_bytes=(unsigned)(len-body_start);
    *out=r;
    return 1;
}

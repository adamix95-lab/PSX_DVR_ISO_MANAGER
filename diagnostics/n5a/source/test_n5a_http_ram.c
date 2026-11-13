#include "n5a_http_ram.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int check(const char *s,int eof) {
    n5a_http_result r={0};
    return n5a_http_check((const uint8_t *)s,strlen(s),eof,&r);
}
int main(void) {
    const char *ok_len=
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
        "Content-Length: 39\r\nConnection: close\r\n\r\n"
        N5A_HTTP_EXPECTED_BODY;
    const char *ok_chunk=
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
        "27\r\n" N5A_HTTP_EXPECTED_BODY "\r\n0\r\n\r\n";
    const char *ok_eof=
        "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n"
        N5A_HTTP_EXPECTED_BODY;
    const char *redirect="HTTP/1.1 302 Found\r\nContent-Length: 39\r\n\r\n"
                         N5A_HTTP_EXPECTED_BODY;
    const char *both="HTTP/1.1 200 OK\r\nContent-Length: 39\r\n"
                     "Transfer-Encoding: chunked\r\n\r\n"
                     N5A_HTTP_EXPECTED_BODY;
    const char *wrong="HTTP/1.1 200 OK\r\nContent-Length: 39\r\n\r\n"
                      "PSX-DVR-ISO-MANAGER N5A HTTPS RAM FAIL\n";
    const char *gzip="HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n"
                     "Content-Length: 39\r\n\r\n" N5A_HTTP_EXPECTED_BODY;
    const char *badchunk="HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                         "ZZ\r\n" N5A_HTTP_EXPECTED_BODY "\r\n0\r\n\r\n";
    const char *wronglen="HTTP/1.1 200 OK\r\nContent-Length: 40\r\n\r\n"
                         N5A_HTTP_EXPECTED_BODY;
    const char *shortbody="HTTP/1.1 200 OK\r\nContent-Length: 39\r\n\r\nPSX";
    const char *duplen="HTTP/1.1 200 OK\r\nContent-Length: 39\r\n"
                       "Content-Length: 39\r\n\r\n" N5A_HTTP_EXPECTED_BODY;
    n5a_http_result result={0};
    assert(N5A_HTTP_EXPECTED_BODY_BYTES==39);
    assert(n5a_http_check((const uint8_t *)ok_len,strlen(ok_len),0,&result)==1);
    assert(result.status==200 && result.body_bytes==39 && result.chunked==0);
    assert(n5a_http_check((const uint8_t *)ok_chunk,strlen(ok_chunk),0,&result)==1);
    assert(result.status==200 && result.body_bytes==39 && result.chunked==1);
    assert(check(ok_eof,0)==0);
    assert(check(ok_eof,1)==1);
    assert(check(redirect,1)<0);
    assert(check(both,1)<0);
    assert(check(wrong,1)<0);
    assert(check(gzip,1)<0);
    assert(check(badchunk,1)<0);
    assert(check(wronglen,1)<0);
    assert(check(shortbody,0)==0);
    assert(check(shortbody,1)<0);
    assert(check(duplen,1)<0);
    assert(check("HTTP/1.1 200 OK\r\nContent-Length: 39\r\n",0)==0);
    assert(check("HTTP/1.1 200 OK\r\nContent-Length: 39\r\n",1)<0);
    assert(n5a_http_check(NULL,1,0,&result)<0);
    assert(n5a_http_check((const uint8_t *)ok_len,strlen(ok_len),0,NULL)<0);
    puts("PASS: HTTP/1.1 TLS RAM parser: fixed 39B body, CL/chunked/EOF, negative cases");
    return 0;
}

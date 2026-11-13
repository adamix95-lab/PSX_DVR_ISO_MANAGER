/*
 * N4C host-only integration: wolfSSL 5.8.2 vs local OpenSSL test server.
 * NO connection to real Internet. NO PS2 ELF. NO native Manager integration.
 *
 * Strictly require a CA-signed server certificate AND DNS hostname.
 * Never disable peer certificate verification or skip the date check.
 * Clock here is CI host OS clock, not the unauthenticated PSX RTC.
 */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/random.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if !defined(WC_RNG_SEED_CB) || !defined(HAVE_HASHDRBG)
#error "N4C requires callback-enabled wolfSSL Hash DRBG"
#endif

/* Host-only: entropy comes from OS, never clock(), xorshift or a key. */
static int os_seed_cb(OS_Seed *unused,byte *dst,word32 n) {
    int fd;
    size_t pos=0;
    (void)unused;
    if(!dst || !n || n>256U) return -1;
    fd=open("/dev/urandom",O_RDONLY);
    if(fd<0) return -1;
    while(pos<(size_t)n) {
        ssize_t got=read(fd,dst+pos,(size_t)n-pos);
        if(got<=0) {close(fd);return -1;}
        pos+=(size_t)got;
    }
    if(close(fd)!=0) return -1;
    return 0;
}

static int do_tls_test(const char *host,const char *cafile,unsigned port) {
    WOLFSSL_CTX *ctx=NULL;
    WOLFSSL *ssl=NULL;
    struct sockaddr_in a;
    int fd=-1,ok=0,rc;
    /* No calls to wolfSSL_connect before verification setup. */
    if(wc_SetSeed_Cb(os_seed_cb)!=0) return -11;
    if(wolfSSL_Init()!=WOLFSSL_SUCCESS) return -12;
    ctx=wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if(!ctx) {ok=-13;goto finish;}
    wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,NULL);
    if(wolfSSL_CTX_load_verify_locations(ctx,cafile,NULL)!=WOLFSSL_SUCCESS) {
        ok=-14;goto finish;
    }
    ssl=wolfSSL_new(ctx);
    if(!ssl) {ok=-15;goto finish;}
    if(wolfSSL_check_domain_name(ssl,host)!=WOLFSSL_SUCCESS) {
        ok=-16;goto finish;
    }
    fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0) {ok=-17;goto finish;}
    memset(&a,0,sizeof(a));
    a.sin_family=AF_INET;
    a.sin_port=htons((uint16_t)port);
    if(inet_pton(AF_INET,"127.0.0.1",&a.sin_addr)!=1 ||
       connect(fd,(struct sockaddr *)&a,sizeof(a))!=0) {
        ok=-18;goto finish;
    }
    if(wolfSSL_set_fd(ssl,fd)!=WOLFSSL_SUCCESS) {ok=-19;goto finish;}
    rc=wolfSSL_connect(ssl);
    if(rc==WOLFSSL_SUCCESS)
        ok=1;  /* the ONLY success condition */
    else
        ok=0;  /* rejected peer */
finish:
    if(ssl) wolfSSL_free(ssl);
    if(fd>=0) close(fd);
    if(ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    wc_SetSeed_Cb(NULL);
    return ok;
}
int main(int argc,char **argv) {
    unsigned port;
    int expected,got;
    if(argc!=5) {
        fprintf(stderr,"Usage: local_tls_client HOST CA_FILE PORT EXPECT_PASS_0_OR_1\n");
        return 2;
    }
    port=(unsigned)strtoul(argv[3],NULL,10);
    expected=atoi(argv[4]);
    if(port<1024U || port>65535U || (expected!=0 && expected!=1))
        return 3;
    got=do_tls_test(argv[1],argv[2],port);
    printf("N4C local wolfSSL: expected=%d actual=%d host=%s\n",
           expected,got,argv[1]);
    if(got<0) return 4; /* setup or TCP failure cannot count as a negative test */
    return got==expected ? 0 : 5;
}

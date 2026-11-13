/* N4G FASTMATH READ-ONLY: isolate MIPS RSA/SP math implementation.
 * Source certificates are PUBLIC official Let's Encrypt examples, pinned
 * in CI to letsencrypt/website commit 736bf97e99c0238082e9a5c43d460af79d03873f.
 * No IOP reset, network, RTC write, HDD, USB, PFS, HTTP, TLS handshake or ART.
 * Fixed November 2026 time is a test fixture ONLY; NEVER a TLS trust clock.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef V12_N4F_HOST
#include <debug.h>
#include <unistd.h>
#endif
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include "n4f_fixtures.h"
#include <wolfssl/wolfcrypt/settings.h>
/* The result is meaningful only if the alternate arithmetic backend is
 * actually selected by BOTH the library and the caller. */
#if defined(WOLFSSL_SP_MATH_ALL) || !defined(USE_FAST_MATH)
#error "N4G must compile with USE_FAST_MATH and without SP_MATH_ALL"
#endif
#ifndef TFM_NO_ASM
#error "N4G must use portable ISO C TFM_NO_ASM"
#endif

#if defined(WOLFSSL_GENSEED_FORTEST)
#error "N4F forbids weak test seeds"
#endif

#ifdef V12_N4F_HOST
#define OUT(...) printf(__VA_ARGS__)
#else
#define OUT(...) scr_printf(__VA_ARGS__)
#endif
static time_t offline_utc_cb(time_t *t) {
  /* Fixed public TEST vector date, independent from PSX depleted RTC.
   * This is NOT usable for live HTTPS or for secure clock authority.
   */
  const time_t fixed=(time_t)1794553146UL; /* 2026-11-13 06:59:06Z */
  if(t) *t=fixed;
  return fixed;
}
static int verify_pair(const unsigned char *ca,unsigned ca_len,
                       const unsigned char *cert,unsigned cert_len,
                       int should_succeed,const char *label) {
  WOLFSSL_CERT_MANAGER *cm;
  int add_rc,rc;
  cm=wolfSSL_CertManagerNew();
  if(!cm){OUT("%s: CERT_MANAGER OOM\n",label);return -801;}
  add_rc=wolfSSL_CertManagerLoadCABuffer(cm,ca,(long)ca_len,
                                         WOLFSSL_FILETYPE_PEM);
  if(add_rc!=WOLFSSL_SUCCESS) {
    OUT("%s: ROOT LOAD FAIL=%d\n",label,add_rc);
    wolfSSL_CertManagerFree(cm);
    return -802;
  }
  /* No network inputs. Verifies ASN.1 signature against public CA. */
  rc=wolfSSL_CertManagerVerifyBuffer(cm,cert,(long)cert_len,
                                     WOLFSSL_FILETYPE_ASN1);
  wolfSSL_CertManagerFree(cm);
  if(should_succeed) {
    OUT("%s: signature verify=%d (%s)\n",label,rc,
        rc==WOLFSSL_SUCCESS?"PASS":"FAIL");
    return rc==WOLFSSL_SUCCESS?0:-803;
  }
  OUT("%s: tamper verify=%d (%s)\n",label,rc,
      rc==WOLFSSL_SUCCESS?"UNSAFE ACCEPT":"REJECT");
  return rc!=WOLFSSL_SUCCESS?0:-804;
}

int main(void) {
  int rc=0,ret=0,init_rc;
  unsigned char tampered[N4F_YR1_DER_CAP];
#ifndef V12_N4F_HOST
  init_scr();
  scr_setCursor(0);
#endif
  OUT("N4G FASTMATH RSA OFFLINE - STAGE 0\n");
  OUT("MATH: TFM PORTABLE C (NO SP/ASM)\n");
  OUT("PUBLIC CERTS, NO NETWORK/HDD WRITES\n");
  init_rc=wolfSSL_Init();
  if(init_rc!=WOLFSSL_SUCCESS){ret=-805;goto finish;}
  if(wc_SetTimeCb(offline_utc_cb)!=0){ret=-806;goto cleanup;}

  OUT("TEST A: X1 verifies cross-signed YR\n");
  rc=verify_pair(n4f_x1_pem,n4f_x1_pem_len,
                 n4f_yr_by_x1_der,n4f_yr_by_x1_der_len,1,"X1->YR");
  if(rc && !ret) ret=rc;

  OUT("TEST B: YR verifies intermediate YR1\n");
  rc=verify_pair(n4f_yr_pem,n4f_yr_pem_len,
                 n4f_yr1_der,n4f_yr1_der_len,1,"YR->YR1");
  if(rc && !ret) ret=rc;

  OUT("TEST C: altered YR1 signature MUST FAIL\n");
  if(n4f_yr1_der_len<32U || n4f_yr1_der_len>N4F_YR1_DER_CAP) {
    if(!ret)ret=-807;
  } else {
    memcpy(tampered,n4f_yr1_der,n4f_yr1_der_len);
    /* Modify the tail of the signed BIT STRING, not ASN.1 metadata. */
    tampered[n4f_yr1_der_len-1U]^=1U;
    rc=verify_pair(n4f_yr_pem,n4f_yr_pem_len,
                   tampered,n4f_yr1_der_len,0,"YR1-TAMPER");
    {volatile unsigned char *p=tampered;
     for(unsigned i=0;i<n4f_yr1_der_len;i++)p[i]=0;}
    if(rc && !ret)ret=rc;
  }
cleanup:
  wc_SetTimeCb(NULL);
  wolfSSL_Cleanup();
finish:
  OUT("N4G FASTMATH OFFLINE RESULT\n");
  OUT("RSA CERT SIGNATURE: %s\n",ret==0?"PASS":"FAIL");
  OUT("NO LIVE HTTPS / CA OVERRIDE / RTC\n");
  OUT("RET=%d\n",ret);
#ifndef V12_N4F_HOST
  sleep(120);
#endif
  return ret==0?0:1;
}

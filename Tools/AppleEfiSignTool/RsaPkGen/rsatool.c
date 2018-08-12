/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can 
be
 * found in the LICENSE file.
 */
/* C port of DumpPublicKey.java from the Android Open source project 
with
 * support for additional RSA key sizes. 
(platform/system/core,git/libmincrypt
 * /tools/DumpPublicKey.java). Uses the OpenSSL X509 and BIGNUM library.
 */
#include <openssl/pem.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "openssl_compat.h"

/* Command line tool to extract RSA public keys from X.509 certificates
 * and output a pre-processed version of keys for use by RSA 
verification
 * routines.
 */
int check(RSA* key) {
  const BIGNUM *n, *e;
  int public_exponent, modulus;
  RSA_get0_key(key, &n, &e, NULL);
  public_exponent = BN_get_word(e);
  modulus = BN_num_bits(n);
  if (public_exponent != 3 && public_exponent != 65537) {
    fprintf(stderr, "WARNING: Public exponent should be 3 or 65537 (but is %d).\n", public_exponent);
  }
  if (modulus != 1024 && modulus != 2048 && modulus != 3072 && modulus 
!= 4096
      && modulus != 8192) {
    fprintf(stderr, "ERROR: Unknown modulus length = %d.\n", modulus);
    return 0;
  }
  return 1;
}
/* Pre-processes and outputs RSA public key to standard out.
 */
void output(RSA* key) {
  int i, nwords;
  const BIGNUM *key_n;
  BIGNUM *N = NULL;
  BIGNUM *Big1 = NULL, *Big2 = NULL, *Big32 = NULL, *BigMinus1 = NULL;
  BIGNUM *B = NULL;
  BIGNUM *N0inv= NULL, *R = NULL, *RR = NULL, *RRTemp = NULL, *NnumBits 
= NULL;
  BIGNUM *n = NULL, *rr = NULL;
  BN_CTX *bn_ctx = BN_CTX_new();
  uint32_t n0invout;
  /* Output size of RSA key in 32-bit words */
  nwords = RSA_size(key) / 4;
  if (-1 == write(1, &nwords, sizeof(nwords)))
    goto failure;
  /* Initialize BIGNUMs */
  RSA_get0_key(key, &key_n, NULL, NULL);
  N = BN_dup(key_n);
  Big1 = BN_new();
  Big2 = BN_new();
  Big32 = BN_new();
  BigMinus1 = BN_new();
  N0inv= BN_new();
  R = BN_new();
  RR = BN_new();
  RRTemp = BN_new();
  NnumBits = BN_new();
  n = BN_new();
  rr = BN_new();
  BN_set_word(Big1, 1L);
  BN_set_word(Big2, 2L);
  BN_set_word(Big32, 32L);
  BN_sub(BigMinus1, Big1, Big2);
  B = BN_new();
  BN_exp(B, Big2, Big32, bn_ctx); /* B = 2^32 */
  /* Calculate and output N0inv = -1 / N[0] mod 2^32 */
  BN_mod_inverse(N0inv, N, B, bn_ctx);
  BN_sub(N0inv, B, N0inv);
  n0invout = BN_get_word(N0inv);
  if (-1 == write(1, &n0invout, sizeof(n0invout)))
    goto failure;
  /* Calculate R = 2^(# of key bits) */
  BN_set_word(NnumBits, BN_num_bits(N));
  BN_exp(R, Big2, NnumBits, bn_ctx);
  /* Calculate RR = R^2 mod N */
  BN_copy(RR, R);
  BN_mul(RRTemp, RR, R, bn_ctx);
  BN_mod(RR, RRTemp, N, bn_ctx);
  /* Write out modulus as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t nout;
    BN_mod(n, N, B, bn_ctx); /* n = N mod B */
    nout = BN_get_word(n);
    if (-1 == write(1, &nout, sizeof(nout)))
      goto failure;
    BN_rshift(N, N, 32); /*  N = N/B */
  }
  /* Write R^2 as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t rrout;
    BN_mod(rr, RR, B, bn_ctx); /* rr = RR mod B */
    rrout = BN_get_word(rr);
    if (-1 == write(1, &rrout, sizeof(rrout)))
      goto failure;
    BN_rshift(RR, RR, 32); /* RR = RR/B */
  }
failure:
  /* Free BIGNUMs. */
  BN_free(N);
  BN_free(Big1);
  BN_free(Big2);
  BN_free(Big32);
  BN_free(BigMinus1);
  BN_free(N0inv);
  BN_free(R);
  BN_free(RRTemp);
  BN_free(NnumBits);
  BN_free(n);
  BN_free(rr);
}
int main(int argc, char* argv[]) {
  int cert_mode = 0;
  FILE* fp;
  X509* cert = NULL;
  RSA* pubkey = NULL;
  EVP_PKEY* key;
  char *progname;
  if (argc != 3 || (strcmp(argv[1], "-cert") && strcmp(argv[1], 
"-pub"))) {
    progname = strrchr(argv[0], '/');
    if (progname)
      progname++;
    else
      progname = argv[0];
    fprintf(stderr, "Usage: %s <-cert | -pub> <file>\n", progname);
    return -1;
  }
  if (!strcmp(argv[1], "-cert"))
    cert_mode = 1;
  fp = fopen(argv[2], "r");
  if (!fp) {
    fprintf(stderr, "Couldn't open file %s!\n", argv[2]);
    return -1;
  }
  if (cert_mode) {
    /* Read the certificate */
    if (!PEM_read_X509(fp, &cert, NULL, NULL)) {
      fprintf(stderr, "Couldn't read certificate.\n");
      goto fail;
    }
    /* Get the public key from the certificate. */
    key = X509_get_pubkey(cert);
    /* Convert to a RSA_style key. */
    if (!(pubkey = EVP_PKEY_get1_RSA(key))) {
      fprintf(stderr, "Couldn't convert to a RSA style key.\n");
      goto fail;
    }
  } else {
    /* Read the pubkey in .PEM format. */
    if (!(pubkey = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL))) {
      fprintf(stderr, "Couldn't read public key file.\n");
      goto fail;
    }
  }
  if (check(pubkey)) {
    output(pubkey);
  }
fail:
  X509_free(cert);
  RSA_free(pubkey);
  fclose(fp);
  return 0;
}


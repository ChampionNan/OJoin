#ifndef ENCDECUTILS_HPP
#define ENCDECUTILS_HPP

#include <cassert>
#include "common/defs.hpp"
#include <random>
#ifdef ENCLAVE_MODE
#include "bearssl_rand.h"
#include "sgx_trts.h"
#endif

#include "bearssl_aead.h"
#include "bearssl_hash.h"
#include <x86intrin.h>
#define memset_s(s, smax, c, n) memset(s, c, n);

#define SGXSD_AES_GCM_IV_SIZE   12
#define SGXSD_AES_GCM_KEY_SIZE  32

namespace eServer {

struct Settings {
  /* A 256 bit key */
  unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
  /* A 128 bit IV */
  unsigned char *iv = (unsigned char *)"0123456789012345";
  size_t iv_len = 16;
  /* Additional data */
  unsigned char *additional = (unsigned char *)"The five boxing wizards jump quickly.";
  /* Buffer for the tag */
  unsigned char tag[16];
};

Settings setting;

void aes_init() {
  static int init = 0;
  if (init == 0) {
    // OpenSSL_add_all_ciphers();
    init = 1;
  }
}

void __attribute__ ((noinline)) sgxsd_br_clear_stack() {
    uint8_t stack[4096];
    memset_s(&stack, sizeof(stack), 0, sizeof(stack));
    _mm256_zeroall();
}

sgx_status_t sgxsd_aes_gcm_run(bool encrypt, const uint8_t p_key[SGXSD_AES_GCM_KEY_SIZE],
                               const void *p_src, uint32_t src_len, void *p_dst,
                               const uint8_t p_iv[SGXSD_AES_GCM_KEY_SIZE],
                               const void *p_aad, uint32_t aad_len,
                               uint8_t p_mac[SGXSD_AES_GCM_KEY_SIZE]) {
    if (p_key == NULL ||
	((p_src == NULL || p_dst == NULL) && src_len != 0) ||
        p_iv == NULL ||
        (p_aad == NULL && aad_len != 0) ||
        p_mac == NULL) {
	return SGX_ERROR_INVALID_PARAMETER;
    }
    br_aes_x86ni_ctr_keys aes_ctx;
    br_aes_x86ni_ctr_init(&aes_ctx, p_key, SGXSD_AES_GCM_KEY_SIZE);
    br_gcm_context aes_gcm_ctx;
    br_gcm_init(&aes_gcm_ctx, &aes_ctx.vtable, &br_ghash_pclmul);
    br_gcm_reset(&aes_gcm_ctx, p_iv, SGXSD_AES_GCM_IV_SIZE);
    if (aad_len != 0) {
        br_gcm_aad_inject(&aes_gcm_ctx, p_aad, aad_len);
    }
    br_gcm_flip(&aes_gcm_ctx);
    if (src_len != 0) {
        memmove(p_dst, p_src, src_len);
        br_gcm_run(&aes_gcm_ctx, encrypt, p_dst, src_len);
    }
    bool tag_res;
    if (encrypt) {
      br_gcm_get_tag(&aes_gcm_ctx, p_mac);
      tag_res = true;
    } else {
      tag_res = br_gcm_check_tag(&aes_gcm_ctx, p_mac);
    }
    sgxsd_br_clear_stack();
    memset_s(&aes_ctx, sizeof(aes_ctx), 0, sizeof(aes_ctx));
    memset_s(&aes_gcm_ctx, sizeof(aes_gcm_ctx), 0, sizeof(aes_gcm_ctx));
    if (tag_res) {
        return SGX_SUCCESS;
    } else {
        if (p_dst != NULL) {
            memset_s(p_dst, src_len, 0, src_len);
        }
        return SGX_ERROR_MAC_MISMATCH;
    }
}


void gcm_encrypt(uint8_t *plaintext, uint8_t *ciphertext,
                         uint64_t plaintextSize,
                         const uint8_t *key = setting.key,
                         uint8_t *iv = setting.iv,
                         uint8_t *tag = setting.tag) {

  aes_init();

  sgxsd_aes_gcm_run(true, key, plaintext, plaintextSize, ciphertext, iv, nullptr, 0, tag);
}



void gcm_decrypt(uint8_t *ciphertext, uint8_t *plaintext,
                         uint64_t ciphertextSize,
                         const uint8_t *key = setting.key,
                         uint8_t *iv = setting.iv,
                         uint8_t *tag = setting.tag) {
  aes_init();
  bool flag = sgxsd_aes_gcm_run(false, key, ciphertext, ciphertextSize, plaintext, iv, nullptr, 0, tag);
  assert(flag == true && "Decryption failed\n"); 
}
}

}

#endif // !ENCDECUTILS_HPP
#ifndef ENCDECUTILS_HPP
#define ENCDECUTILS_HPP

#include <cstdio>
#include <assert.h>

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
    OpenSSL_add_all_ciphers();
    init = 1;
  }
}

void gcm_encrypt(uint8_t *plaintext, uint8_t *ciphertext,
                         uint64_t plaintextSize,
                         const uint8_t *key = setting.key,
                         uint8_t *iv = setting.iv,
                         uint8_t *tag = setting.tag) {

  aes_init();

  size_t enc_length = ((plaintextSize + 15) / 16) * 16;

  int actual_size = 0, final_size = 0;
  EVP_CIPHER_CTX *e_ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
  EVP_CIPHER_CTX_set_padding(e_ctx, 0);
  EVP_EncryptInit(e_ctx, EVP_aes_256_gcm(), key, iv);

  EVP_EncryptUpdate(e_ctx, ciphertext, &actual_size, plaintext, plaintextSize);
  int ok = EVP_EncryptFinal(e_ctx, &ciphertext[actual_size], &final_size);
  assert(final_size <= enc_length);
  EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
  EVP_CIPHER_CTX_free(e_ctx);
  assert(ok == 1);
}

void gcm_decrypt(uint8_t *ciphertext, uint8_t *plaintext,
                         uint64_t ciphertextSize,
                         const uint8_t *key = setting.key,
                         uint8_t *iv = setting.iv,
                         uint8_t *tag = setting.tag) {
  aes_init();

  // UNDONE(): Make sure this is using aesni
  
  int actual_size = 0, final_size = 0;
  EVP_CIPHER_CTX *d_ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
  EVP_CIPHER_CTX_set_padding(d_ctx, 0);
  EVP_DecryptInit(d_ctx, EVP_aes_256_gcm(), key, iv);
  EVP_DecryptUpdate(d_ctx, plaintext, &actual_size, ciphertext, ciphertextSize);
  EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
  int ok;
  ok = EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);
  EVP_CIPHER_CTX_free(d_ctx);
  assert(ok == 1);
}

}

#endif // !ENCDECUTILS_HPP
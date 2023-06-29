#ifndef ENCDECUTILS_HPP
#define ENCDECUTILS_HPP

#include "sgx_eid.h"
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "sgx_thread.h"
#include "sgx_tseal.h"

namespace eServer {

struct Settings {
  /* A 256 bit key */
  const uint8_t *key = (unsigned char *)"01234567890123456789012345678901";
  /* A 128 bit IV */
  const uint8_t *iv = (unsigned char *)"0123456789012345";
  size_t iv_len = 16;
};

Settings setting;

void gcm_encrypt(uint8_t *plaintext, uint8_t *ciphertext,
                         uint64_t plaintextSize,
                         const uint8_t *key = setting.key,
                         const uint8_t *iv = setting.iv) {
  sgx_aes_state_handle_t *state;
  sgx_aes_gcm128_enc_init(key, iv, setting.iv_len, NULL, 0, state);
  sgx_aes_gcm128_enc_update(plaintext, plaintextSize, ciphertext, state);
  sgx_aes_gcm_close(state);
}

void gcm_decrypt(uint8_t *ciphertext, uint8_t *plaintext,
                         uint64_t ciphertextSize,
                         const uint8_t *key = setting.key,
                         const uint8_t *iv = setting.iv) {
  sgx_aes_state_handle_t *state;
  sgx_aes_gcm128_enc_init(key, iv, setting.iv_len, NULL, 0, state);
  sgx_aes_gcm128_enc_update(ciphertext, ciphertextSize, plaintext, state);
  sgx_aes_gcm_close(state);
}
}

#endif // !ENCDECUTILS_HPP
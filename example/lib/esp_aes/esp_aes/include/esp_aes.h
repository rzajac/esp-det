#ifndef ESP_AES_H
#define ESP_AES_H

#include <c_types.h>

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES.
#define Nb 4
// The number of 32 bit words in a key.
#define Nk 4
// Key length in bytes [128 bit].
#define KEYLEN 16
// The number of rounds in AES Cipher.
#define Nr 10

// jcallan@github points out that declaring Multiply as a function
// reduces code size considerably with the Keil ARM compiler.
// See this link for more information: https://github.com/kokke/tiny-AES128-C/pull/3
#ifndef MULTIPLY_AS_A_FUNCTION
  #define MULTIPLY_AS_A_FUNCTION 0
#endif

/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/

// Array holding the intermediate results during decryption.
typedef uint8_t state_t[4][4];

uint32_t esp_aes_encrypt(uint8_t *output, uint8_t *input, uint32_t length, const uint8_t *key, const uint8_t *iv);
uint32_t esp_aes_decrypt(uint8_t *output, uint8_t *input, uint32_t length, const uint8_t *key, const uint8_t *iv);

#endif //ESP_AES_H

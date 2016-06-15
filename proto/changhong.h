#ifndef __CHANGHONG_H__
#define __CHANGHONG_H__

#include <easy_setup.h>

#define CHANGHONG_KEY_STRING_LEN (16)
#define CHANGHONG_NONCE_PAD_LEN (16)

/* param for changhong */
typedef struct {
    uint8 key_bytes[CHANGHONG_KEY_STRING_LEN];  /* key string for decoding */
    uint8 random_bytes[CHANGHONG_NONCE_PAD_LEN]; /* random bytes */
} changhong_param_t;

/* changhong result */
typedef struct {
    easy_setup_result_t es_result;
    uint8 sec_mode; /* sec mode */
} changhong_result_t;

void changhong_get_param(void* p);
void changhong_set_result(const void* p);

int changhong_set_key(const char* key);

int changhong_get_sec_mode(uint8* sec);

#endif /* __CHANGHONG_H__ */

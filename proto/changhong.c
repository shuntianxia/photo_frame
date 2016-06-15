#include <string.h>

#include <changhong.h>

static changhong_param_t g_changhong_param = {
    .key_bytes = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    .random_bytes = {
        'b', '7', '0', 'e', 'c', '5', '6', '3',
        '2', '6', 'b', '2', '4', 'f', '3', '9',
    },
};
static changhong_result_t g_changhong_result;

void changhong_get_param(void* p) {
    memcpy(p, &g_changhong_param, sizeof(g_changhong_param));
}

void changhong_set_result(const void* p) {
    memcpy(&g_changhong_result, p, sizeof(g_changhong_result));
    LOGE("changhong_set_result: state: %d\n", g_changhong_result.es_result.state);
}

int changhong_set_key(const char* key) {
    if (strlen(key) < sizeof(g_changhong_param.key_bytes)) {
        LOGE("invalid key length: %d < %d\n", 
                strlen(key), sizeof(g_changhong_param.key_bytes));
        return -1;
    }

    memcpy(g_changhong_param.key_bytes, key, sizeof(g_changhong_param.key_bytes));

    return 0;
}

int changhong_get_sec_mode(uint8* sec) {
    if (g_changhong_result.es_result.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    *sec = g_changhong_result.sec_mode;
    return 0;
}

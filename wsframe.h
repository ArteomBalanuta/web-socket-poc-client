#include <stdbool.h>

typedef struct ws_frame_standard {
    unsigned char fin_byte;
    unsigned char payload_length;
    unsigned char masking_key[4];
    unsigned char payload[8096];
} __attribute__((packed)) ws_frame_standard ;

typedef struct ws_frame_extended {
    unsigned char fin_byte;
    unsigned char payload_length_flag; // 254 extended flag
    uint16_t payload_length;
    unsigned char masking_key[4];
    unsigned char payload[8096];
} __attribute__((packed)) ws_frame_extended;
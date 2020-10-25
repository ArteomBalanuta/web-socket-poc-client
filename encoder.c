#include <string.h>

void websocket_encode(unsigned char *string, char *mask_byte_arr, unsigned char *encoded){
    int j = 0;
    for(int i=0; i < strlen(string); i++){
        j = i % 4;
        encoded[i] = string[i] ^  mask_byte_arr[j];
    }
}

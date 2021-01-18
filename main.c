#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <poll.h>
#include "encoder.h"
#include "wsframe.h"
#include "notify.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <signal.h>

// HTTP payloads
#define UPGRADE_PAYLOAD "GET %s HTTP/1.1\r\n\
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:78.0) Gecko/20100101 Firefox/78.0\r\n\
Host: %s\r\n\
Accept: */*\r\n\
Accept-Language: en-US,en;q=0.5\r\n\
Accept-Encoding: gzip, deflate\r\n\
Sec-WebSocket-Version: 13\r\n\
Sec-WebSocket-Extensions: permessage-deflate\r\n\
Sec-WebSocket-Key: iu62cI/zQ0qwWXOzlhfDEA==\r\n\
Connection: keep-alive, Upgrade\r\n\
Pragma: no-cache\r\n\
Cache-Control: no-cache\r\n\
Upgrade: websocket\r\n\r\n"

// Error messages
#define MSG_PARAMS_ERROR "Hostname ws-uri tls-port - should be passed as parameters\n Example: example.org /ws-uri 443\n"
#define MSG_SSL_CREATE_ERROR "Error creating SSL context\n"
#define MSG_CONNECTION_ERROR "Error attempting to connect\n"
#define MSG_SET_BIO_DESCRIPTOR_ERROR "Could not set bio descriptor\n"
#define MSG_UPGRADE_PROTOCOL_ERROR "Upgrade protocol failed\n"
#define MSG_POLL_ERROR "Error on poll\n"
#define MSG_WS_SEND_ERROR "Send WS payload failed\n"

// Variable short-cuts
#define WEBSOCKET_MASK_BYTES {8, 57, 135, 10}
#define HTTPS ":https"
#define HTTP1_1 ptr_http11
#define WEB_SOCKET_BYTE (char) 129
#define BUFFER_SIZE 4096
#define MAX_STANDARD_PAYLOAD_SIZE 126
#define HELP_CMD "~help"
#define QUIT_CMD "~quit"

char *ptr_http11 = "HTTP/1.1 101";

struct ws_frame_standard *ptr_ws_recv_frame;
struct ws_frame_extended *ptr_ws_recv_frame_extended;

SSL_CTX *ctx;
SSL *ssl;
BIO *cbio;
int socket_desc, len = 0, status = 0, upgrade_request_max_size = 550;
int hostname_max_size = 50;

struct pollfd pfds[1];
char rec_buffer[BUFFER_SIZE] = {0};
unsigned char init_mask_byte_arr[] = WEBSOCKET_MASK_BYTES;

int swap_highest_bit(int payload){
    return payload | 0b10000000;
}

void ws_frame_set_mask(unsigned char * ptr_mask, const char * ptr_to_mask_arr){
    ptr_mask[0] = ptr_to_mask_arr[0];
    ptr_mask[1] = ptr_to_mask_arr[1];
    ptr_mask[2] = ptr_to_mask_arr[2];
    ptr_mask[3] = ptr_to_mask_arr[3];
}

void send_ws_frame(BIO *cbio, char *text_payload) {
    int payload_len = strlen(text_payload);

    struct ws_frame_standard ws_standard_frame;
    struct ws_frame_extended ws_extended_frame;

    int ws_frame_size;
    char *ptr_head;
    char *ptr_payload;

    bool is_standard_ws_frame = payload_len <= MAX_STANDARD_PAYLOAD_SIZE;
    if (is_standard_ws_frame) {
        ptr_head = &ws_standard_frame.fin_byte;
        ptr_payload = &ws_standard_frame.payload;
        int payload_len_with_masked_bit_set = swap_highest_bit(payload_len);

        ws_standard_frame.fin_byte = 129;
        ws_standard_frame.payload_length = payload_len_with_masked_bit_set;
        ws_frame_set_mask(&ws_standard_frame.masking_key[0], &init_mask_byte_arr[0]);
        ws_frame_size = 6 + payload_len;
    } else {
        ptr_head = &ws_extended_frame.fin_byte;
        ptr_payload = &ws_extended_frame.payload;

        ws_extended_frame.fin_byte = 129;
        ws_extended_frame.payload_length_flag = 254;
        ws_extended_frame.payload_length = ntohs(payload_len);
        ws_frame_set_mask(&ws_extended_frame.masking_key[0], &init_mask_byte_arr[0]);
        ws_frame_size = 8 + payload_len;
    }
    websocket_encode(text_payload, &init_mask_byte_arr[0], ptr_payload);

    bool is_sent = BIO_write(cbio, ptr_head, ws_frame_size);
    if (!is_sent) {
        printf(MSG_WS_SEND_ERROR);
    }
}

void print_ws(char *rec_buffer, int len) {
    unsigned char *ptr_head = &ptr_ws_recv_frame->fin_byte;
    unsigned char *ptr_payload = &ptr_ws_recv_frame->payload;
    unsigned char *ptr_size = &ptr_ws_recv_frame->payload_length;

    bool is_extended_frame = rec_buffer[1] == 126;
    if (is_extended_frame) {
        ptr_head = &ptr_ws_recv_frame_extended->fin_byte;
        ptr_payload = &ptr_ws_recv_frame_extended->payload;
    }
    memcpy(ptr_head, rec_buffer, len);

    int messageSize = 100;
    unsigned char * message = calloc(messageSize, sizeof(unsigned char));    
    
    bool isNotEmptyMessage = false;
    if (is_extended_frame) {
        uint16_t *ptr_size_e = &ptr_ws_recv_frame_extended->payload_length;

        *ptr_size_e = ntohs(*ptr_size_e);
        // Server's packets are 4k long,
        // iterating using ws payload length my cause read out of bounds
        printf("ws payload size: %d\n", *ptr_size_e);
        if (*ptr_size_e > 4096) {
            *ptr_size_e = 4000;
        }

        for (int i = 0; i < *ptr_size_e; i++) {
            // -4 bytes offset because server's payload does not have mask
            printf("%c", ptr_payload[i - 4]);
	
	    // Copy 100 chars to notification msg
	    if(i < messageSize - 1) {
	       message[i] = ptr_payload[i - 4];
	    }
        }

	if (*ptr_size_e != 0) {
	    isNotEmptyMessage = true;
	}

    } else {
        for (int i = 0; i < *ptr_size; i++) {
            printf("%c", ptr_payload[i - 4]);
        }
    }
    printf("\n");

    // send notification with the message to UI    
    if (isNotEmptyMessage) {
        send_notification("hack.chat", message);
    }
    free(message);
}

void print_recv_buffer(char *rec_buffer, int len) {
    for (int i = 0; i < len; i++) {
        printf("%c", rec_buffer[i]);
    }
    printf("\n");
}

int setup_http_upgrade_request(char* ptr_upgrade_request, char* ptr_hostname, char* ws_uri) {
    sprintf(ptr_upgrade_request, UPGRADE_PAYLOAD, ws_uri, ptr_hostname);
 return (int) strlen(ptr_upgrade_request);
}

void init_ssl_library_and_error_strings(){
   SSL_load_error_strings();
   SSL_library_init();
}

void validate_input_arguments_count(const int *argc){
    bool paramsNotSet = *argc != 4;
    if (paramsNotSet) {
        printf(MSG_PARAMS_ERROR);
        exit(0);
    }
}

void create_ssl_context() {
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        fprintf(stderr, MSG_SSL_CREATE_ERROR);
        ERR_print_errors_fp(stderr);
        exit(1);
    }
}

void setup_bio_connection_params(char *hostname, char* port){
    cbio = BIO_new_ssl_connect(ctx);
    BIO_get_ssl(cbio, &ssl);

    strcat(hostname, HTTPS);

    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(cbio, hostname);
    BIO_set_conn_port(cbio, port);
}

void bio_connect(){
    if (BIO_do_connect(cbio) <= 0) {
        printf(MSG_CONNECTION_ERROR);
        ERR_print_errors_fp(stderr);
        BIO_free_all(cbio);
        SSL_CTX_free(ctx);
        exit(1);
    }
}

void bio_set_socket_descriptor(){
    bool isFdSet = BIO_get_fd(cbio, &socket_desc) > 0;
    if (!isFdSet) {
        printf(MSG_SET_BIO_DESCRIPTOR_ERROR);
        exit(1);
    }
}

void pollfd_setup(){
    pfds[0].fd = socket_desc;
    pfds[0].events = POLLIN;
}

void send_upgrade_request(char *upgrade_request, int upgrade_request_size){
    bool is_sent = BIO_write(cbio, upgrade_request, upgrade_request_size) >= 0;
    if (!is_sent) {
        printf(MSG_UPGRADE_PROTOCOL_ERROR);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    validate_input_arguments_count(&argc);

    char *hostname = calloc(hostname_max_size, sizeof(unsigned char));
    memcpy(hostname, argv[1], hostname_max_size - 1);

    char *ws_uri = calloc(50, sizeof(unsigned char));
    memcpy(ws_uri, argv[2], 50 - 1);

    char *upgrade_request = calloc(upgrade_request_max_size, sizeof(unsigned char));
    int upgrade_request_size = setup_http_upgrade_request(upgrade_request, hostname, ws_uri);

    init_ssl_library_and_error_strings();
    create_ssl_context();

    char *port = argv[3];
    setup_bio_connection_params(hostname, port);
    bio_connect();
    free(hostname);

    bio_set_socket_descriptor();
    pollfd_setup();

    send_upgrade_request(upgrade_request, upgrade_request_size);
    free(upgrade_request);

    ptr_ws_recv_frame = calloc(9013, sizeof(char));
    ptr_ws_recv_frame_extended = calloc(9013, sizeof(char));

    char *ptr_quit = calloc(6, sizeof(unsigned char));
    memcpy(ptr_quit, QUIT_CMD, 6);

    char *ptr_help = calloc(6, sizeof(unsigned char));
    memcpy(ptr_help, HELP_CMD, 6);

    bool is_connected = true;
    bool is_cmd_help = false;
    bool is_cmd_quit = false;
    bool is_upgraded = false;
    bool is_ws_struct = false;

    char *join_json = "{ \"cmd\": \"join\", \"channel\": \"lab\", \"nick\": \"refactor\" }";
    char *help_payload = "{ \"cmd\": \"chat\", \"text\": \"you are fine!\"}";
    char *quit_payload = "{ \"cmd\": \"chat\", \"text\": \"good luck!\"}";

    while (is_connected) {
        status = poll(pfds, (uint) 1, -1);
        switch (status) {
            case POLL_ERR:
                printf(MSG_POLL_ERROR);
                break;
            case POLLIN:
                len = BIO_read(cbio, rec_buffer, BUFFER_SIZE - 1);
                // Make sure buffer is null-terminated so that string functions may be used on it.
                rec_buffer[len] = 0;

                is_upgraded = strstr(rec_buffer, HTTP1_1) != NULL;
                if (is_upgraded) {
                    send_ws_frame(cbio, join_json);
                    break;
                }

                is_ws_struct = rec_buffer[0] == WEB_SOCKET_BYTE;
                if (is_ws_struct) {
                    print_ws(rec_buffer, len);

                    is_cmd_help = strstr(rec_buffer, ptr_help) != NULL;
                    if (is_cmd_help) {
                        send_ws_frame(cbio, help_payload);
                    }

                    is_cmd_quit = strstr(rec_buffer, ptr_quit) != NULL;
                    if (is_cmd_quit) {
                        send_ws_frame(cbio, quit_payload);
                        is_connected = false;
                        break;
                    }
                } else {
                    print_recv_buffer(rec_buffer, len);
                }
                break;
            default:
                break;
        }
    }
    free(ptr_help);
    free(ptr_quit);
    free(ptr_ws_recv_frame);
    free(ptr_ws_recv_frame_extended);

    memset(rec_buffer, 0, sizeof(rec_buffer));
    BIO_free_all(cbio);
    SSL_CTX_free(ctx);
    return 0;
}

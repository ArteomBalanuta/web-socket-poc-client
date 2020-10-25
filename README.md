Web socket client poc which uses OpenSSL for SSL support.

How to Build: 

gcc encoder.c main.c -o main.out -lssl -lcrypto 

How to use:

./main.out hostname.org /ws-uri 443

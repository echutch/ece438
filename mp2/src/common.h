#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

void diep(char *s) {
    perror(s);
    exit(1);
}

// header types
enum {
    SYN = 1,
    SYNACK,
    DATA,
    ACK,
    FIN,
    FINACK
};

#define HDR_SIZE 12
typedef struct __attribute__((packed)) {
    uint16_t type;
    uint16_t len;
    uint32_t seq;
    uint32_t ack;
} hdr_t;

#define MAX_PAYLOAD (1472 - HDR_SIZE)
// 1472B payload won't be fragmented by UDP

typedef struct {
    hdr_t h;
    uint8_t payload[MAX_PAYLOAD];
} packet_t;


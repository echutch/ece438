/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */

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
#include <poll.h>
#include "common.h"

struct sockaddr_in si_other;
int s, slen;

static long curr_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

static void send_ctrl(uint16_t type, uint32_t seq, uint32_t ack) {
    hdr_t h;
    h.type = type;
    h.len = 0;
    h.seq = seq;
    h.ack = ack;

    if (sendto(s, &h, sizeof(h), 0, (struct sockaddr*)&si_other, slen) == -1)
        diep("sendto");
}

// send a single packet
static void send_data(FILE *fp, uint32_t idx, unsigned long long total) {
    packet_t p;
    unsigned long long offset = (unsigned long long)idx * MAX_PAYLOAD;
    size_t want;

    if (total - offset < MAX_PAYLOAD) {
        want = (size_t)(total - offset);
    } else {
        want = MAX_PAYLOAD;
    }

    fseeko(fp, offset, SEEK_SET);
    size_t n = fread(p.payload, 1, want, fp);

    p.h.type = DATA;
    p.h.seq = idx;
    p.h.ack = 0;
    p.h.len = n;

    if (sendto(s, &p, sizeof(hdr_t) + n, 0, (struct sockaddr*)&si_other, slen) == -1)
        diep("sendto");
}

#define RTO_MS 200

// using a constant timeout, just keep sending until handshake is complete
// this approach might need to be adjusted for congestion avoidance with multiple connections? 
// but it should be fine since it's low overhead

static void handshake(char teardown) {
    struct pollfd pfd = {.fd = s, .events = POLLIN};

    uint16_t initiate, recv_ack;
    if (!teardown) {
        initiate = SYN;
        recv_ack = SYNACK;
    } else {
        initiate = FIN;
        recv_ack = FINACK;
    }

    while (1) {
        // send SYN every timeout until received SYNACK
        send_ctrl(initiate, 0, 0);
        long time_sent = curr_ms();

        while (1) {
            long remaining = RTO_MS - (curr_ms() - time_sent);
            if (remaining <= 0) break;

            int ready = poll(&pfd, 1, (int)remaining);
            if (ready < 0) diep("poll");
            if (ready == 0) break;

            hdr_t in;
            ssize_t r = recvfrom(s, &in, sizeof(in), 0, NULL, NULL);
            if (in.type == recv_ack) {
                printf("received SYN/FIN ACK\n"); // debug
                send_ctrl(ACK, 0, 0);
                return;
            }
        }
    }
}

// TCP states
enum {
    SS = 1, // slow start
    CA, // congestion avoidance
    FR // fast recovery
};

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

	/* Determine how many bytes to transfer */

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


	/* Send data and receive acknowledgements on s*/


    /* Vars
        - seq (base/next): byte sequence number, identify payloads
        - CW: congestion window, number of bytes to send per seq
        - SST: slow start threshold, when CW >= SST transition to AIMD from slow start
        - state: slow start, AIMD, or fast recovery - determines CW, DupACK 
        - dupACK: counter for duplicate packet ACKs - detect packet loss
    */


    /* 1. Establish connection with receiver*/
    handshake(0);

    // set isn = 0. could use a random number like in lecture but that seemed unnecessary
    // if needed later can change handshake to send with SYN packet 

    /* 2. Start transmitting and receiving ACKs*/

    // integer ceiling division to get total packet count
    uint32_t total_pkts = ((bytesToTransfer + MAX_PAYLOAD - 1) / MAX_PAYLOAD);  
    
    uint32_t base = 0; // start of window
    uint32_t next = 0; // current packet sent in window

    uint32_t cwnd = 1;
    uint32_t sst = 64;
    uint16_t dupack = 0;
    uint16_t state = SS;

    struct pollfd pfd = {.fd = s, .events = POLLIN};


    // TODO: main handler here

    while (base < total_pkts) {
        // send: send cwnd packets - this will be acked by the receiving end
        while (next < total_pkts && next < base + cwnd) {
            send_data(fp, next, bytesToTransfer);
            next++;
        }

        // poll until timeout or ACK received


         // receive: receive ACKS, update state
    // ACK or timeout will update TCP state machine
    }

    /* 3. Teardown */
    handshake(1);


    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * Testing on localhost: ./reliable_sender 127.0.0.1 8080 test.txt 4
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}



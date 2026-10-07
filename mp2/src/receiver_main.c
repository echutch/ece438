/* 
 * File:   receiver_main.c
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
#include "common.h"

struct sockaddr_in si_me, si_other;
int s, slen;

static void send_ctrl(uint16_t type, uint32_t seq, uint32_t ack) {
    hdr_t h;
    h.type = type;
    h.len = 0;
    h.seq = seq;
    h.ack = ack;

    if (sendto(s, &h, sizeof(h), 0, (struct sockaddr*)&si_other, slen) == -1)
        diep("sendto");
}


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


	/* Now receive data and send acknowledgements */    

    // open file we are writing to
    FILE *fp = fopen(destinationFile, "wb");
    if (!fp)
        diep("fopen");

    uint32_t next_seq = 0;
    packet_t in;
    socklen_t addrlen;

    char fin = 0;

    while (1) {
        addrlen = sizeof(si_other);
        ssize_t r = recvfrom(s, &in, sizeof(in), 0, (struct sockaddr*)&si_other, &addrlen);

        uint16_t type = in.h.type;

        if (type == DATA) {
            // TODO: main handler here

            // will need a buffer for receiving out of order packets - need to decide on size (rwnd - flow control window) 

            // next_seq determines if a packet is consumed -> written to file
            // else if packet arrived early hold in buffer until seq number matches

            // send next_seq ack to sender
        } else if (type == SYN) {
            send_ctrl(SYNACK, 0, 0);
            printf("received SYN\n"); // debug
        } else if (type == FIN) {
            send_ctrl(FINACK, 0, 0);
            fin = 1;
            printf("received FIN\n"); // debug
        } else if (type == ACK) {
            printf("received ACK\n"); // debug
            if (fin) {
                break;
            }
        }
    }

    fclose(fp);
    close(s);
	printf("%s received.\n", destinationFile);
    return;
}

/*
 * Testing on localhost: ./reliable_receiver 8080 out.txt
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}


/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: ./http_client http://hostname[:port]/path/to/file\n");
	    exit(1);
	}

	char* url = argv[1];
	char hostport[MAXDATASIZE];
	char hostname[MAXDATASIZE];
	char port[6] = "80";
	char path[MAXDATASIZE];

	// extract client args from url
	const char* prefix = "http://";
	if (strncmp(url, prefix, strlen(prefix)) != 0) {
		fprintf(stderr, "url must start with http://\n");
		exit(1);
	}

	const char* rest_url = url + strlen(prefix);
	const char* first_slash = strchr(rest_url, '/');
	if (first_slash == NULL) {
		fprintf(stderr, "url must include file path\n");
		exit(1);
	}

	size_t hostport_len = first_slash - rest_url;

	strncpy(hostport, rest_url, hostport_len);
	hostport[hostport_len] = '\0';

	char* colon = strchr(hostport, ':');
	if (colon != NULL) {
		*colon = '\0';
		strncpy(hostname, hostport, sizeof(hostname) - 1);
		strncpy(port, colon + 1, sizeof(port) - 1);

		if (strlen(port) == 0) {
			fprintf(stderr, "missing port number after :\n");
		}
	} else {
		strncpy(hostname, hostport, sizeof(hostname) - 1);
	}

	if (strlen(hostname) == 0) {
		fprintf(stderr, "missing hostname\n");
		exit(1);
	}

	strncpy(path, first_slash, sizeof(path) - 1);

	printf("hostname: %s\n", hostname);
	printf("port: %s\n", port);
	printf("path: %s\n", path);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	char get_req[MAXDATASIZE];
	int req_size = snprintf(get_req, sizeof(get_req), "GET %s HTTP/1.1\r\n\r\n", path);
	if (send(sockfd, get_req, req_size, 0) == -1) {
		printf("reached\n");
		perror("send");
		close(sockfd);
		exit(0);
	}


	// if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	//     perror("recv");
	//     exit(1);
	// }

	int total_size = 0;
	char* resp = NULL;

	while ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
		resp = realloc(resp, total_size + numbytes);
		memcpy(resp + total_size, buf, numbytes);
		total_size += numbytes;
	}
	close(sockfd);

	printf("%s\n", resp);

	// // split into header and body
	// char* end_header = NULL;
	// for (size_t i = 0; i + 3 < total_size; i++) {
	// 	if (resp[i] == '\r' && resp[i+1] == '\n' &&
	// 		resp[i+2] == '\r' && resp[i+3] == '\n') {
	// 			end_header = resp + i + 4;
	// 			break;
	// 		}
	// }

	// // writeback to output
	// size_t body_len = total_size - (end_header - resp);
	// FILE* output = fopen("output", "wb");
	// fwrite(end_header, 1, body_len, output);
	// fclose(output);

	return 0;
}


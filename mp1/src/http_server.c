/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 500 // max number of bytes we can get at once 

void sigchld_handler(int s)
{
	(void)s;
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int send_fd(int fd, const char *buf, size_t len) {
	size_t total_sent = 0;
	while (total_sent < len) {
		ssize_t n = send(fd, buf + total_sent, len - total_sent, 0);
		if (n == -1) {
			perror("send");
			return -1;
		}
		total_sent += n;
	}
	return 0;
}

void get_handler(int new_fd) {
	// int numbytes, total_size;
	int numbytes;
	char buf[MAXDATASIZE];
	// char* resp = NULL;

	if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		return;
	}
	buf[numbytes] = '\0';

	printf("%s\n", buf);

	char path[MAXDATASIZE];
	char *start = buf + 4; // skip "GET " (with space)
	char *end = strchr(start, ' ');
	size_t path_length = end - start;

	memcpy(path, start, path_length);
	path[path_length] = '\0';

	char *filename = path + 1;

	if (access(filename, F_OK) == -1) {
		// if path doesn't exist, return 404
		const char *header = "HTTP/1.1 404 Not Found\r\n\r\n";
		send_fd(new_fd, header, strlen(header));
		return;
	}

	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		perror("fopen");
		// can't be opened (or anything else), return 400
		const char *header = "HTTP/1.1 400 Bad Request\r\n\r\n";
		send_fd(new_fd, header, strlen(header));
		return;
	}

	const char *header = "HTTP/1.1 200 OK\r\n\r\n";
	if (send_fd(new_fd, header, strlen(header)) == -1) {
		fclose(file);
		return;
	}

	char filebuf[4096];
	size_t n;
	while ((n = fread(filebuf, 1, sizeof(filebuf), file)) > 0) {
		if (send_fd(new_fd, filebuf, n) == -1) {
			break;
		}
	}

	fclose(file);
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	if (argc != 2) {
		fprintf(stderr, "usage: ./http_server port");
		exit(1);
	}

	char* port = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			// if (send(new_fd, "Hello, world!", 13, 0) == -1)
			// 	perror("send");
			get_handler(new_fd);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


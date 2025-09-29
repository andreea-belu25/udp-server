#include "lib.h"

int fd; // Server socket descriptor

void display(message_server message) {
	struct sockaddr_in sock_addr;
	sock_addr.sin_addr.s_addr = message.ip;
	
	printf("%s:%d - %s - ", inet_ntoa(sock_addr.sin_addr), ntohs(message.port), message.topic);
	
	if (message.type == TCP_INT) {
		unsigned int networkNumber = *(unsigned int *)((char *)message.data + 1);
		unsigned int number = ntohl(networkNumber);
		
		if ((unsigned char)(*message.data) == 1) {
			number = -number;
		}
		
		printf("INT - %d\n", number);
		return;
	}
	
	if (message.type == TCP_SHORT_REAL) {
		unsigned short *nr = (unsigned short *)message.data;
		double number = (ntohs(*nr)) / (double)100;
		
		printf("SHORT_REAL - %.2f\n", number);
		return;
	}
	
	if (message.type == TCP_FLOAT) {
		unsigned char sign = message.data[0];
		unsigned int *nr = (unsigned int *)((char *)message.data + 1);
		unsigned char p = message.data[5];
		double number = (ntohl(*nr)) / pow(10, p);
		
		if (sign == 1)
			number = -number;
		
		printf("FLOAT - %.*f\n", p, number);
		return;
	}
	
	if (message.type == TCP_STRING) {
		printf("STRING - %s\n", message.data);	
	}
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	unsigned short port;
	sscanf(argv[3], "%hu", &port);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(fd < 0, "socket failed");
	
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	int rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");
	
	int enable = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");
	
	// Disable Nagle's algorithm
	rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
	DIE(rc < 0, "nagle");
	
	rc = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");
	
	// Send client ID to the server
	rc = send(fd, argv[1], strlen(argv[1]), 0);
	DIE(rc < 0, "send");
	
	struct pollfd fds[2];
	fds[1].fd = fd;
	fds[1].events = POLLIN;
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	
	while (1) {
		int rc = poll(fds, 2, -1);
		DIE(rc < 0, "poll");
		
		if (fds[1].revents & POLLIN) {
			message_server message;
			memset(&message, 0, sizeof(message_server));
			
			int rc = receive_message(fd, &message);
			DIE(rc < 0, "recv");
			
			// Server closed connection
			if (message.type == TCP_QUIT)
				exit(0);
			
			display(message);
		}
		
		if (fds[0].revents & POLLIN) {
			char buffer[1024];
			memset(buffer, 0, sizeof(buffer));
			
			int rc = read(STDIN_FILENO, buffer, sizeof(buffer));
			DIE(rc < 0, "read");
			
			char cmd[1024], topic[1024];
			sscanf(buffer, "%s %s", cmd, topic);
			
			if (strcmp(cmd, "exit") == 0) {
				message_client message;
				message.type = CLIENT_QUIT;
				send_message_client(fd, message);
				exit(0);
			}
			
			if (strcmp(cmd, "subscribe") == 0 || strcmp(cmd, "unsubscribe") == 0) {
				message_client message;
				message.type = strcmp(cmd, "subscribe") == 0 ? CLIENT_SUBSCRIBE : CLIENT_UNSUBSCRIBE;
				strcpy(message.topic, topic);
				send_message_client(fd, message);
				
				printf("%s topic %s\n", strcmp(cmd, "subscribe") == 0 ? "Subscribed to" : "Unsubscribed from", topic);
			}
		}
	}
	
	close(fd);
	return 0;
}

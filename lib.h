#ifndef _lib_H
#define _lib_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <math.h>
#include <string.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif

#define TOPIC_SIZE 51
#define DATA_SIZE 1500
#define TCP_INT 0
#define TCP_SHORT_REAL 1
#define TCP_FLOAT 2
#define TCP_STRING 3
#define TCP_QUIT 15

#define CLIENT_QUIT 0
#define CLIENT_SUBSCRIBE 1
#define CLIENT_UNSUBSCRIBE 2

typedef struct {
	uint8_t type;
	char topic[TOPIC_SIZE];
} message_client;

typedef struct {
	char type;
	unsigned short port;
	unsigned int ip;
	char topic[TOPIC_SIZE];
	char data[DATA_SIZE];
} message_server;

typedef struct {
	char *topic;
	char **client_ids;
	int ids_count;
} topic;

typedef struct {
	topic *topics;
	int count;
} topics;

int receive_message(int file_descriptor, void *buffer);
void send_message_server(int file_descriptor, message_server message);
void send_message_client(int file_descriptor, message_client message);

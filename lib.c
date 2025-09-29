#include "lib.h"

int receive(int file_descriptor, void *buffer, int len) {
	int left_to_receive = len;
	char *buff = buffer;
	
	while(left_to_receive > 0) {
		int rc = recv(file_descriptor, buff, left_to_receive, 0);
		
		if (rc < 0) 
			return len - left_to_receive;
		
		buff += rc;
		left_to_receive -= rc;
	}
	
	return len - left_to_receive;
}

int send_info(int file_descriptor, void *buffer, int len) {
	int left_to_send = len;
	char *buff = buffer;
	
	while (left_to_send > 0) {
		int rc = send(file_descriptor, buff, left_to_send, 0);
		
		if (rc < 0)
			return len - left_to_send;
		
		buff += rc;
		left_to_send -= rc;
	}
	
	return len - left_to_send;
}

int receive_message(int file_descriptor, void *buffer) {
	short len;
	
	// Get the length of the message first
	receive(file_descriptor, &len, sizeof(short));
	
	// Get the message afterwards
	return receive(file_descriptor, buffer, len);
}

void send_message_server(int file_descriptor, message_server message) {
	short len = 1;
	
	if (message.type != TCP_QUIT) {
		len = 1 + 2 + 4 + TOPIC_SIZE + 1;
		
		if (message.type == TCP_INT)
			len = len + 5;
		
		if (message.type == TCP_SHORT_REAL)
			len = len + 2;
		
		if (message.type == TCP_FLOAT)
			len = len + 6;
		
		if (message.type == TCP_STRING)
			len = len + strlen(message.data) + 1; // +1 for null terminator
	}
	
	// Send length first
	send_info(file_descriptor, &len, sizeof(short));
	
	// Send message afterwards
	send_info(file_descriptor, (void *)&message, len);
}

void send_message_client(int file_descriptor, message_client message) {
	short len = 1;
	
	if (message.type != CLIENT_QUIT)
		len = len + strlen(message.topic) + 1; // +1 for null terminator
	
	// Send length first
	send_info(file_descriptor, &len, sizeof(short));
	
	// Send message afterwards
	send_info(file_descriptor, (void *)&message, len);
}

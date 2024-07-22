#include "lib.h"

int connected_clients;
char **client_ids;
int *client_fds;

topics existing_topics;

int client_already_connected(struct pollfd *poll_fds, char *client_id) {
	int connected = 0;
	for (int i = 0; i < connected_clients; i++) {
		if (strcmp(client_ids[i], client_id) == 0) {
			connected = 1;
		}
	}
	return connected;
}

void kill_client(int fd) {
	message_server message;
	message.type = TCP_QUIT;
	send_message_server(fd, message);
}

void kill_all_clients() {
	for (int i = 0; i < connected_clients; i++)
		kill_client(client_fds[i]);
}

int obtain_client_index(int fd) {
	int index = -1;
	for (int j = 0; j < connected_clients; j++) {
		if (client_fds[j] == fd) {
			index = j;
		}
	}

	return index;
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	client_ids = calloc(1, sizeof(char *));
	client_fds = calloc(1, sizeof(int));
	existing_topics.topics = calloc(1, sizeof(topic));
	existing_topics.count = 0;

	uint16_t port;
	sscanf(argv[1], "%hu", &port);
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp < 0, "udp socket");
	int enable = 1;
	int rc = setsockopt(udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");
	rc = bind(udp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");

	int tcp = socket(AF_INET, SOCK_STREAM, 0);
	rc = setsockopt(tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");
	rc = setsockopt(tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt");
	rc = bind(tcp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");
	rc = listen(tcp, 5);
	DIE(rc < 0, "tcp listen");

	struct pollfd *poll_fds = calloc(3, sizeof(struct pollfd));
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = tcp;
	poll_fds[1].events = POLLIN;
	poll_fds[2].fd = udp;
	poll_fds[2].events = POLLIN;

	while (1) {
		int active_fds = 3 + connected_clients;
		int rc = poll(poll_fds, active_fds, -1);
		DIE(rc < 0, "poll");

		if (poll_fds[0].revents & POLLIN) {
			char buffer[1024];
			memset(buffer, 0, sizeof(buffer));
			int rc = read(STDIN_FILENO, buffer, sizeof(buffer));
			DIE(rc < 0, "read");
			if (strncmp(buffer, "exit", strlen("exit")) == 0) {
				kill_all_clients();
				exit(0);
			}
		}

		// parse from the back in case a client requests disconnect
		for (int i = active_fds - 1; i >= 3; i--) {
			if (poll_fds[i].revents & POLLIN) {
				message_client message;
				receive_message(poll_fds[i].fd, &message);
				int client_index = obtain_client_index(poll_fds[i].fd);

				if (message.type == CLIENT_QUIT) {
					if (client_index != -1) {
						int fds_count = 3 + connected_clients;
						int client_fd = client_fds[client_index];
						printf("Client %s disconnected.\n", client_ids[client_index]);
						for (int j = client_index; j < connected_clients - 1; j++) {
							client_fds[j] = client_fds[j + 1];
							free(client_ids[j]);
							client_ids[j] = calloc(strlen(client_ids[j + 1]) + 1, sizeof(char));
							strcpy(client_ids[j], client_ids[j + 1]);
						}
						connected_clients -= 1;
						if (connected_clients > 0) {
							client_fds = realloc(client_fds, connected_clients * sizeof(int));
							client_ids = realloc(client_ids, connected_clients * sizeof(char *));
						}

						for (int j = i; j < fds_count - 1; j++) {
							poll_fds[j].fd = poll_fds[j + 1].fd;
							poll_fds[j].events = poll_fds[j + 1].events;
							poll_fds[j].revents = poll_fds[j + 1].revents;
						}
						poll_fds = realloc(poll_fds, (fds_count - 1) * sizeof(struct pollfd));
						close(client_fd);
					}
				} else {
					if (client_index != -1) {
						int topic_index = -1;
						for (int j = 0; j < existing_topics.count; j++) {
							if (strcmp(existing_topics.topics[j].topic, message.topic) == 0) {
								topic_index = j;
							}
						}
						if (topic_index == -1) {
							existing_topics.topics = realloc(existing_topics.topics, (existing_topics.count + 1) * sizeof(topic));
							existing_topics.topics[existing_topics.count].topic = calloc(strlen(message.topic) + 1, sizeof(char));
							existing_topics.topics[existing_topics.count].client_ids = calloc(1, sizeof(char *));
							existing_topics.topics[existing_topics.count].ids_count = 0;
							strcpy(existing_topics.topics[existing_topics.count].topic, message.topic);
							topic_index = existing_topics.count;
							existing_topics.count++;
						}
						int index = -1;
						for (int j = 0; j < existing_topics.topics[topic_index].ids_count; j++) {
							if (strcmp(existing_topics.topics[topic_index].client_ids[j], client_ids[client_index]) == 0) {
								index = j;
							}
						}

						if (message.type == CLIENT_SUBSCRIBE) {
							if (index == -1) {
								existing_topics.topics[topic_index].client_ids = realloc(existing_topics.topics[topic_index].client_ids, (existing_topics.topics[topic_index].ids_count + 1) * sizeof(char *));
								existing_topics.topics[topic_index].client_ids[existing_topics.topics[topic_index].ids_count] = calloc(strlen(client_ids[client_index]) + 1, sizeof(char));
								strcpy(existing_topics.topics[topic_index].client_ids[existing_topics.topics[topic_index].ids_count], client_ids[client_index]);
								existing_topics.topics[topic_index].ids_count++;
							}
						} else if (message.type == CLIENT_UNSUBSCRIBE) {
							if (index != -1) {
								for (int j = index; j < existing_topics.topics[topic_index].ids_count - 1; j++) {
									existing_topics.topics[topic_index].client_ids[j] = calloc(strlen(existing_topics.topics[topic_index].client_ids[j + 1]) + 1, sizeof(char));
									strcpy(existing_topics.topics[topic_index].client_ids[j], existing_topics.topics[topic_index].client_ids[j + 1]);
								}
								existing_topics.topics[topic_index].ids_count--;
								existing_topics.topics[topic_index].client_ids = realloc(existing_topics.topics[topic_index].client_ids, (existing_topics.topics[topic_index].ids_count + 1) * sizeof(char *));
							}
						}
					}
				}
			}
		}

		if (poll_fds[2].revents & POLLIN) {
			struct sockaddr_in udp_client;
			socklen_t len;
			char buff[1 + TOPIC_SIZE + 1 + DATA_SIZE + 1];
			memset(buff, 0, sizeof(1 + TOPIC_SIZE + 1 + DATA_SIZE + 1));
			int bytes_received = recvfrom(udp, buff, sizeof(buff), 0, (struct sockaddr *)&udp_client, &len);
			DIE(bytes_received < 0, "recvfrom");
			
			message_server message;
			memset(&message, 0, sizeof(message_server));
			message.type = buff[TOPIC_SIZE - 1];
			message.port = udp_client.sin_port;
			message.ip = udp_client.sin_addr.s_addr;
			memcpy(message.topic, buff, TOPIC_SIZE);
			memcpy(message.data, buff + TOPIC_SIZE, bytes_received - TOPIC_SIZE);
			int topic_index = -1;
			for (int j = 0; j < existing_topics.count; j++) {
				if (strcmp(existing_topics.topics[j].topic, message.topic) == 0) {
					topic_index = j;
				}
			}
			if (topic_index != -1) {
				for (int j = 0; j < existing_topics.topics[topic_index].ids_count; j++) {
					int index = -1;
					for (int k = 0; k < connected_clients; k++) {
						if (strcmp(client_ids[k], existing_topics.topics[topic_index].client_ids[k]) == 0) {
							index = k;
						}
					}
					if (index != -1) {
						send_message_server(client_fds[index], message);
					}
				}
			}
		}

		if (poll_fds[1].revents & POLLIN) {
			struct sockaddr_in client_address;
			socklen_t length = sizeof(client_address);
			int client_fd = accept(tcp, (struct sockaddr *)&client_address, &length);
			DIE(client_fd < 0, "accept");

			char client_id[20];
			memset(client_id, 0, 20);
			int rc = recv(client_fd, client_id, 20, 0);
			DIE(rc < 0, "receive error");
			if (client_already_connected(poll_fds, client_id)) {
				// disconnect
				kill_client(client_fd);
				printf("Client %s already connected.\n", client_id);
			} else {
				// connect
				client_ids = realloc(client_ids, (connected_clients + 1) * sizeof(char *));
				client_ids[connected_clients] = calloc(strlen(client_id) + 1, sizeof(char));
				strcpy(client_ids[connected_clients], client_id);

				client_fds = realloc(client_fds, (connected_clients + 1) * sizeof(int));
				client_fds[connected_clients] = client_fd;

				int fds_count = 3 + connected_clients;
				poll_fds = realloc(poll_fds, (fds_count + 1) * sizeof(struct pollfd));
				poll_fds[fds_count].fd = client_fd;
				poll_fds[fds_count].events = POLLIN;

				connected_clients += 1;
				printf("New client %s connected from %s:%d\n", client_id, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			}
		}		
	}

	return 0;
}
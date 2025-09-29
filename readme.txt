Server:
	- Opens a UDP socket to receive UDP messages and a TCP socket to receive TCP connection requests from clients
	- Starts a poll to listen on these sockets, on stdin, and on sockets from clients for messages
	- If it receives a message on the TCP socket, it obtains the client's ID and checks whether that ID is already connected or not. If not connected, it adds that socket to poll; otherwise it closes that client.
	- If it receives a message on the UDP socket, it checks if anyone is subscribed to the message's topic and that client is also connected. If yes, it forwards the message.
	- If it receives a command on STDIN, it checks if the command is "exit". If yes, it closes both the server and all connected clients.
	- If it receives a message on a socket belonging to a client, it checks if the command is:
		- "exit": in which case it disconnects the client.
		- "subscribe topic": in which case it subscribes the client to that topic, if not already subscribed.
		- "unsubscribe topic": in which case it unsubscribes the client from that topic, if subscribed.

Client:
	- At startup, opens a TCP socket to communicate with the server and sends its ID.
	- Starts poll to listen for messages both from the server and from STDIN.
	- If it receives "exit" at stdin, it closes the client.
	- If it receives a message from the server:
		- In case the message indicates closing the client, it closes.
		- Otherwise it displays the message.

Message framing:
	- When sending, messages are framed, having their length attached at the beginning.
	- When receiving, first the message length is read, and then the message of that length is read.

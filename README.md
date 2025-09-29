Server:
- Opens a UDP socket to receive UDP messages and a TCP socket to receive TCP connection requests from clients
- Starts a poll to listen on these sockets, on stdin, and on sockets from clients for messages
- If it receives a message on the TCP socket:
  - Obtains the client's ID
  - Checks whether that ID is already connected or not
  - If not connected, adds that socket to poll
  - If already connected, closes that client
- If it receives a message on the UDP socket:
  - Checks if anyone is subscribed to the message's topic
  - Checks if that client is also connected
  - If yes, forwards the message
- If it receives a command on STDIN:
  - Checks if the command is "exit"
  - If yes, closes both the server and all connected clients
- If it receives a message on a socket belonging to a client, checks if the command is:
  - "exit": disconnects the client
  - "subscribe topic": subscribes the client to that topic, if not already subscribed
  - "unsubscribe topic": unsubscribes the client from that topic, if subscribed

Client:
- At startup:
  - Opens a TCP socket to communicate with the server
  - Sends its ID
- Starts poll to listen for messages both from the server and from STDIN
- If it receives "exit" at stdin:
  - Closes the client
- If it receives a message from the server:
  - If the message indicates closing the client, it closes
  - Otherwise displays the message

Message framing:
- When sending:
  - Messages are framed with their length attached at the beginning
- When receiving:
  - First the message length is read
  - Then the message of that length is read

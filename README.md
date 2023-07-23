# Chatclient

## Overview

A chatclient that communicates with the provided chatserver. The chatclient sends messages to the chatserver, which then broadcasts the message to all other clients connected to the server. Similarly, the client will be able to receive messages directly from the server, thus enabling the user to read messages sent by all other clients on the system. 
s
## Functionality

The program is run from the command line and takes in two arguments, the server IP and port number. The server IP is in IPv4 format and should be input in dot-decimal notation, and the port number should be within the range [1024, 65535]. The program uses 'inet_pton()' to check the IP address for validity and exits in failure if the address is not valid, along with an appropriate error message. Same with the port number.

After parsing the command line arguments, the program prompts the user for a username. If username is blank or exceeds the MAX_NAME_LEN, the program reprompts for a valid username. After a valid username is entered, the client then prints a welcome message and attempts to connect to the server.

### Connecting to the Server

The process is as follows:

1. Create a TCP Socket
2. Connect to the server
3. Receive the welcome message from the server

    If the number of bytes received is 0, that means the server closed
    the connection on the client and an error message is printed and the program exits in failure.

4. Next, print a new line, the welcome message received from the server, and two more lines.

5. Send the username to the server.

If any of these steps fail, an error message is printed to stderr and the program exits in failure.

### Sending and Receiving Messages

All messages received from the server are stored in the inbuf buffer and all outbound messages from STDIN_FILENO are stored in the outbuf buffer. All strings from the server are null-terminated before being sent to the client, and all strings sent to the server are null-terminated.

After a connection to the server is established, the program enters a loop, waiting for either input from the user or activity from the client socket. The program uses 'fd_set' and 'select' to determine if there is activity on either of the two file descriptors, STDIN_FILENO and the client socket. If there is activity on:

1. `STDIN_FILENO`

    The chat client should work if the user types directly into the terminal or if a file is redirected
    through stdin. If using file redirection, the first line will be the username. The rest of the lines
    will each contain a message to be sent to the server. A new line marks the end of a message and must
    be sent before reading additional characters from the file. If a message is too long (no new line
    character is found before visiting MAX_MSG_LEN characters, print to standard error:

        "Sorry, limit your message to 1 line of at most %d characters.\n"
    
    where %d is `MAX_MSG_LEN`. The rest of the characters up to EOF or a new line character is consumed, so that
    message is discarded.

    Otherwise, the message is sent to the server, regardless of its contents.
    If the message is equal to "bye", "Goodbye.\n" is printed and the client is shut down.

2. `Client Socket`

    Data is received from the socket and stored in the inbuf buffer. If the number
    of bytes received is -1 and errno != EINTR, the user is warned with the following:

        "Warning: Failed to receive incoming message."
    
    If the number of bytes received is 0, it means that server abruptly broke the 
    connection with the client, as in the server crashed or perhaps the network failed.
    The client is shut down with the following message:

        "\nConnection to server has been lost.\n"

    If the received message is equal to "bye", "\nServer initiated shutdown.\n" is printed and the program exits.

    Otherwise, the message from the server is printed.

The socket is closed before the program terminates. 

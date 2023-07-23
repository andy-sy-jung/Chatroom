#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

volatile sig_atomic_t running = true;

void catch_signal(int sig){
    running = false;
}

int handle_stdin(int client_socket) {
    enum parse_string_t status;
    status = get_string(outbuf, MAX_MSG_LEN);
    if(status == NO_INPUT && strlen(outbuf) != 0){
        return EXIT_FAILURE;
    }
    else if(status == END_OF_FILE){
        return EXIT_FAILURE;
    }
    else if(status == NO_INPUT){
        return EXIT_SUCCESS;
    }
    else if(status == TOO_LONG){
        fprintf(stderr, "Sorry, limit your message to 1 line of at most %d characters.\n",
                MAX_MSG_LEN);
        return EXIT_SUCCESS;
    }
    else if(status == OK){
        if(strcmp(outbuf, "bye") == 0){
            printf("Goodbye.\n");
            if(send(client_socket, outbuf, strlen(outbuf) + 1, 0) == -1){
               fprintf(stderr, "Error: Failed to send message '%s' to server. %s.\n", outbuf,
                      strerror(errno));
            } 
            return EXIT_FAILURE;
        }
        else{
            if(send(client_socket, outbuf, strlen(outbuf) + 1, 0) == -1){
                fprintf(stderr, "Error: Failed to send message '%s' to server. %s.\n", outbuf,
                        strerror(errno));
            }
            return EXIT_SUCCESS;
        }
    }
    return EXIT_SUCCESS;
}

int handle_client_socket(int client_socket) {
    int bytes_recvd = recv(client_socket, inbuf, BUFLEN + 1, 0);
    if(bytes_recvd == -1 && errno != EINTR){
        fprintf(stderr, "Warning: Failed to recieve incoming message from server. %s.\n",
                strerror(errno));
        return EXIT_SUCCESS;
    }
    else if(bytes_recvd == 0){
        fprintf(stderr, "\nConnection to server has been lost.\n");
        return EXIT_FAILURE;
    }
    else{
        inbuf[bytes_recvd] = '\0';
        if(strcmp(inbuf, "bye") == 0){
            printf("\nServer initiated shutdown.\n");
            return EXIT_FAILURE;
        }
        printf("\n%s\n", inbuf);
        return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}

void display_usage(char *prog_name){
    fprintf(stderr, "Usage: %s <server IP> <port>\n", prog_name);
}

int main(int argc, char **argv) {
    if(argc != 3){
        display_usage(argv[0]);
        return EXIT_FAILURE;
    }
    char *ip_str = argv[1];
    char *port = argv[2];
    int port_num;
    int client_socket, bytes_recvd, ip_conversion;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    memset(&server_addr, 0, addrlen);
    ip_conversion = inet_pton(AF_INET, ip_str, &server_addr.sin_addr);
    if (ip_conversion == 0){
        fprintf(stderr, "IP address '%s' is invalid.\n", ip_str);
        return EXIT_FAILURE;
    }
    else if(ip_conversion < 0){
        fprintf(stderr, "Error: Failed to convert IP address. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if(!parse_int(port, &port_num, "port number")){
        return EXIT_FAILURE;
    }

    if((port_num < 1024) || (port_num > 65535)){
        fprintf(stderr, "Error: port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;
    }

    //set up signal handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = catch_signal;
    if(sigaction(SIGINT, &action, NULL) == -1){
        fprintf(stderr, "Error: Failed to register signal handler. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }


    username[0] = '\0';

    while(strlen(username) == 0){
        //if stdin is from the keyboard, print the prompt
        //otherwise, skip
        if(isatty(fileno(stdin))){
            printf("Enter a username: ");
            fflush(stdout);
        }
        char c;
        int index = 0;
        //originally used fgets to get username
        //but using read (in get_string() in handle_stdin) after
        //seems to make it not work well with stdin redirection
        //so just read from stdin character by character
        while(1){
            ssize_t result = read(STDIN_FILENO, &c, 1);
            if(result == -1 && errno != EINTR){
                fprintf(stderr, "Error: read() failed. %s.\n", strerror(errno));
                return EXIT_FAILURE;
            }
            if(errno == EINTR){
                printf("\n");
                return EXIT_FAILURE;
            }
            outbuf[index++] = c;
            if(c == '\n'){
                break;
            }
        }
        outbuf[index-1] = '\0';
        if(strlen(outbuf) <= MAX_NAME_LEN){
            strcpy(username, outbuf);
        }
        else{
            if(isatty(fileno(stdin))){
                printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
            }
            else{
                printf("Sorry, username in file (for stdin redirection) is too long.\n");
                return EXIT_FAILURE;
            }
        }
    }
    
    printf("Hello, %s. Let's try to connect to the server.\n", username);

    //creating client socket
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    //server_addr.sin_addr is already set from inet_pton() above
    
    //establish the connection to the server
    if(connect(client_socket, (struct sockaddr *)&server_addr, addrlen) < 0){
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    //receive welcome message from server
    if((bytes_recvd = recv(client_socket, inbuf, MAX_MSG_LEN + 1, 0)) <= 0){
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    //null-terminate
    inbuf[bytes_recvd] = '\0';

    printf("\n");
    printf("%s\n", inbuf);
    printf("\n");

    //send username to server
    if(send(client_socket, username, strlen(username) + 1, 0) == -1){
        fprintf(stderr, "Error: Failed to send username to server. %s.\n", strerror(errno));
        close(client_socket);
        return EXIT_FAILURE;
    }

    //main part of the chatclient
    //looping forever till server shuts down or
    //client disconnects
    fd_set sockset;
    int max_socket;
    while(running){
        FD_ZERO(&sockset);
        FD_SET(client_socket, &sockset);
        FD_SET(STDIN_FILENO, &sockset);
        max_socket = client_socket;

        //if stdin is from keyboard, print out the username
        if(isatty(fileno(stdin))){
            fprintf(stdout, "[%s]: ", username);
            fflush(stdout);
        }

        if(select(max_socket + 1, &sockset, NULL, NULL, NULL) < 0 && errno != EINTR){
            fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
            close(client_socket);
            return EXIT_FAILURE;
        }

        //if there is activity on stdin, call handle_stdin()
        if(running && FD_ISSET(STDIN_FILENO, &sockset)){
            if(handle_stdin(client_socket) == EXIT_FAILURE){
                close(client_socket);
                return EXIT_FAILURE;
            }
        }

        //if there is activity from the server, call handle_client_socket
        if(running && FD_ISSET(client_socket, &sockset)){
            if(handle_client_socket(client_socket) == EXIT_FAILURE){
                close(client_socket);
                return EXIT_FAILURE;
            }
        }

    }

    printf("\n");



    return EXIT_SUCCESS; 


}

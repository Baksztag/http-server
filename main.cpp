#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

//TODO Add config file support
//TODO Log server activity to *.log files
//TODO Handle signals
//TODO Create function for threads to handle http requests
//TODO Create server loop to redirect requests to separate threads
//TODO Implement GET
//TODO Implement POST
//TODO Implement standard response codes
//TODO Add error handling

//W prezentacji zaprezentowac dzialajaca obsluge bledow
//oraz w POST obsluzyc skrypt CGI

int main() {
    struct addrinfo name, *res;
    int server_socket;
    memset(&name, 0, sizeof(name));
    name.ai_family = AF_UNSPEC;
    name.ai_socktype = SOCK_STREAM;
    name.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, "8000", &name, &res);
    if ((server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        printf("An error occurred while trying to create socket\n");
        exit(EXIT_FAILURE);
    }
    if ((bind(server_socket, res->ai_addr, res->ai_addrlen)) < 0) {
        printf("An error occurred while trying to bind the socket to port\n");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    if ((listen(server_socket, 5)) < 0) {
        printf("An error occurred while trying to start listening\n");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    sockaddr_storage address;
    socklen_t address_len = sizeof(address);
    int client_socket;
    if ((client_socket = accept(server_socket, (sockaddr *) &address, &address_len)) < 0) {
        perror("Accept");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    char request[1024];
    recv(client_socket, request, 1024, 0);
    printf("Request from client: %s\n", request);
    send(client_socket, "abc", 1024, 0);

    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
    return 0;
}
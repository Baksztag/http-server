#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define STATIC_FILE_PATH "../public"

//TODO Add config file support
//TODO Add error handling

//W prezentacji zaprezentowac dzialajaca obsluge bledow
//oraz w POST obsluzyc skrypt CGI

int server_socket = -1;
FILE *log = NULL;

typedef struct _client {
    int client_socket;
} client;

char **get_config() {
    FILE *config_file = fopen("../config", "r");
    if (config_file == NULL) {
        perror("Config");
        exit(EXIT_FAILURE);
    }
    char *attribute;
    size_t len = 0;
    char **result = new char*[2];
    int j = 0;
    while (getline(&attribute, &len, config_file) != -1) {
        int i = 0;
        while (attribute[i] != '=') {
            ++i;
        }
        result[j] = attribute + i + 1;
        ++j;
    }
    return result;
}

void log_message(char *message) {
    time_t now;
    time(&now);
    fprintf(log, "%s. Time: %s", message, ctime(&now));
}

void shutdown(int sig_no) {
    printf("Shuttingthe server down\n");
    log_message((char *) "Shutting down server");
    if (close(server_socket) < 0) {
        perror("Server close");
    }
    if (fclose(log)) {
        perror("close log file");
    }
    exit(0);
}

int set_up_server_socket(char *port) {
    struct addrinfo name, *res;
    int server_socket;
    memset(&name, 0, sizeof(name));
    name.ai_family = AF_UNSPEC;
    name.ai_socktype = SOCK_STREAM;
    name.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &name, &res);
    if ((server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        printf("An error occurred while trying to create socket\n");
        return -1;
    }
    if ((bind(server_socket, res->ai_addr, res->ai_addrlen)) < 0) {
        printf("An error occurred while trying to bind the socket to port\n");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        return -1;
    }
    if ((listen(server_socket, 5)) < 0) {
        printf("An error occurred while trying to start listening\n");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        return -1;
    }
    return server_socket;
}

int accept_connection(int server_socket) {
    sockaddr_storage address;
    socklen_t address_len = sizeof(address);
    int client_socket;
    if ((client_socket = accept(server_socket, (sockaddr *) &address, &address_len)) < 0) {
        perror("Accept");
        return -1;
    }
    return client_socket;
}

void headers_200_OK(int client_socket) {
    char buffer[1024];
    strcpy(buffer, "HTTP/1.1 200 OK\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: text/html; charset=UTF-8\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
//    sprintf(buffer, "Content-Length: 145\r\n");
//    send(client_socket, buffer, strlen(buffer), 0);
    strcpy(buffer, "\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
}

void headers_501_method_not_implemented(int client_socket) {
    char buffer[1024];
    strcpy(buffer, "HTTP/1.1 501 Method Not Implemented\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: text/html; charset=UTF-8\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
    strcpy(buffer, "\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
}

void headers_404_not_found(int client_socket) {
    char buffer[1024];
    strcpy(buffer, "HTTP/1.1 404 Not Found\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: text/html; charset=UTF-8\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
    strcpy(buffer, "\r\n");
    send(client_socket, buffer, strlen(buffer), 0);
}

void serve_file(int client_socket, char *path, void headers(int)) {
    printf("File path: %s\n", path);
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // NOT FOUND
        printf("Not found\n");
        char not_found_path[256];
        sprintf(not_found_path, "%s/not_found.html", STATIC_FILE_PATH);
        file = fopen(not_found_path, "r");
        headers_404_not_found(client_socket);
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char file_buf[length];
        fread(file_buf, 1, (size_t) length, file);
        send(client_socket, file_buf, (size_t) length, 0);
    } else {
        headers(client_socket);
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char file_buf[length];
        fread(file_buf, 1, (size_t) length, file);
        send(client_socket, file_buf, (size_t) length, 0);
    }
    fclose(file);
}

void unimplemented(int client) {
    char path[256];
    sprintf(path, "%s/method_not_implemented.html", STATIC_FILE_PATH);
    serve_file(client, path, headers_501_method_not_implemented);
}

void *handle_request(void *clt) {
    client *new_client = (client *) clt;
    int client_socket = new_client->client_socket;
    free(clt);
    char request[1024];
    char method[256];
    recv(client_socket, request, 1024, 0);
    printf("Request: %s\n", request);
    int i = 0;
    int j = 0;
    while (!isspace(request[j]) && (i < sizeof(method) - 1)) {
        method[i] = request[j];
        ++i;
        ++j;
    }
    method[i] = '\0';

    ++i;
    char *headers = request + i;
    char *query_string = (char *) "";
    while (headers[i] != '\0') {
        while (headers[i] != '\n') {
            ++i;
        }
        if (headers[i + 1] == '\0') {
            break;
        } else if (headers[i + 1] == '\r' && headers[i + 2] == '\n') {
            query_string = headers + i + 3;
            break;
        }
        ++i;
    }

    char log_msg[1024];
    sprintf(log_msg, "Received request %s with query string '%s'", method, query_string);
    log_message(log_msg);

    char method_env_var[256];
    char query_env_var[256];
    char content_len_env_var[256];
    sprintf(method_env_var, "%s", method);
    sprintf(query_env_var, "%s", query_string);
    sprintf(content_len_env_var, "%d", (int) strlen(query_env_var));
    setenv("REQUEST_METHOD", method_env_var, 1);
    setenv("QUERY_STRING", query_env_var, 1);
    setenv("CONTENT_LENGTH", content_len_env_var, 1);

    // UNIMPLEMENTED
    if (strcasecmp(method, "get") != 0 && strcasecmp(method, "post") != 0) {
        sprintf(log_msg, "Responding with %s unimplemented", method);
        log_message(log_msg);
        printf("Unimplemented\n");
        unimplemented(client_socket);
        return NULL;
    }

    i = 0;
    char url[256];
    while (isspace(request[j]) && (j < sizeof(request))) {
        ++j;
    }
    while (!isspace(request[j]) && (i < sizeof(url) - 1) && (j < sizeof(request))) {
        url[i] = request[j];
        ++i;
        ++j;
    }
    url[i] = '\0';

//  GET
    if (strcasecmp(method, "get") == 0) {
        sprintf(log_msg, "Sending response for %s %s", method, url);
        log_message(log_msg);
        char path[256];
        if (strcmp(url, "/") == 0) {
            sprintf(path, "%s/index.html", STATIC_FILE_PATH);
            serve_file(client_socket, path, headers_200_OK);
        } else {
            sprintf(path, "%s%s.html", STATIC_FILE_PATH, url);
            serve_file(client_socket, path, headers_200_OK);
        }
    }
//    POST
    else {
        sprintf(log_msg, "Executing cgi for client %d", client_socket);
        log_message(log_msg);
        char path[512];
        sprintf(path, "/Users/jkret/Studia/Projekty/HTTP/http-server%s", url);
        pid_t cgi = fork();
        if (cgi == 0) {
            printf("%s\n", url);
            FILE *cgi_stream = popen(path, "r");
            if (cgi_stream == NULL) {
                perror("popen");
                return NULL;
            }
            char line[256];
            strcpy(line, "HTTP/1.1 200 OK\r\n");
            send(client_socket, line, strlen(line), 0);
            while (fgets(line, 256, cgi_stream) != NULL) {
                send(client_socket, line, strlen(line), 0);
            }
            pclose(cgi_stream);
            return NULL;
        } else {
            int status;
            waitpid(cgi, &status, 0);
            sprintf(log_msg, "cgi script for client %d executed", client_socket);
            log_message(log_msg);
        }
    }
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    signal(SIGINT, shutdown);
    char *port = (char *) "8000";
    if ((server_socket = set_up_server_socket(port)) < 0) {
        printf("An error occurred while trying to set up server socket\n");
        exit(EXIT_FAILURE);
    }
    log = fopen("server.log", "a");
    if (log == NULL) {
        printf("An error occurred while trying to create/open the log file\n");
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    char log_msg[1024];
    sprintf(log_msg, "Server started. Listening on port %s", port);
    log_message(log_msg);

    printf("Listening on port %s\n", port);
    while (1) {
        int client_socket;
        if ((client_socket = accept_connection(server_socket)) < 0) {
            printf("An error occurred while trying to accept a connection\n");
        }

        pthread_t thread;
        client *new_client = (client *) malloc(sizeof(client));
        new_client->client_socket = client_socket;
        if (pthread_create(&thread, NULL, handle_request, (void *) new_client) != 0) {
            perror("Create thread");
        }
        sprintf(log_msg, "Accepted new connection on socket %d", client_socket);
        log_message(log_msg);
    }
}

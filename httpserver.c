#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define exit(N) {fflush(stdout); fflush(stderr); _exit(N); }


#define RMAX 4096
#define HMAX 1024
#define BMAX 1024
#define FDMAX 16
#define PMAX 1024

static ssize_t RSIZE = 0;
static char request[RMAX];
static int HSIZE = 0;
static char header[HMAX+1];
static int BSIZE = 0;
static char body[BMAX+1];
static int BUF = 7;
static char buffer[BMAX +1] = "<empty>";


static const char err404_response[]= "< ERROR 404 here >";
static const char ping_req[]= "GET /ping HTTP/1.1\r\n\r\n";
static const char echo_req[]= "GET /echo HTTP/1.1\r\n";
static const char write_req[]= "POST /write HTTP/1.1\r\n";
static const char read_req[]= "GET /read HTTP/1.1\r\n";
static const char ping_body[] = "pong";
static const char OK2[]= "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n";
static const char stats_req[] = "GET /stats HTTP/1.1\r\n\r\n";
static const char bad_400[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
static const char lost_404[] = "HTTP/1.1 404 Not Found\r\n\r\n";
static const char large_413[] = "HTTP/1.1 413 Request Entity Too Large\r\n\r\n";
static int get_port(void);
static int Socket(int namespace, int style, int protocol);
static void Bind(int sockfd,  struct sockaddr *server, socklen_t length);
static void Listen(int sockfd, int qlen);
static int Accept(int sockfd, struct sockaddr *addr, socklen_t *length_ptr);

static int Socket(int namespace, int style, int protocol)
{
    int sockfd = socket(namespace, style, protocol);
    if (sockfd < 0) {
        perror("sock check foo");
        exit(1);
    }
    return sockfd;
}

static void Bind(int sockfd,  struct sockaddr *server, socklen_t length)
{
    if (bind(sockfd, server, length)< 0) {
        perror("bind");
        exit(1);
    }
}

static void Listen(int sockfd, int qlen)
{
    if (listen(sockfd, qlen) < 0) {
        perror("listen");
        exit(1);
    }
}

static int Accept(int sockfd, struct sockaddr *addr, socklen_t *length_ptr)
{
    int newfd = accept(sockfd, addr, length_ptr);
    if (newfd < 0) {
        perror("accept");
        exit(1);
    }
    return newfd;
}

ssize_t Recv(int socket, void* buffr, size_t size, int flags)
{
    ssize_t ret = recv(socket, buffr, size, flags);
    if(ret < 0){
        perror("recv");
        exit(1);
    }
    return ret;
}

ssize_t Send(int socket, const void* buffr, size_t size, int flags)
{
    ssize_t ret = send(socket, buffr, size, flags);
    if(ret < 0){
        perror("send");
        exit(1);
    }
    return ret;
}

void Connect(int socket, struct sockaddr *server, socklen_t length)
{
    if(connect(socket, server, length) < 0){
        perror("connect");
        exit(1);
    }
}

static void send_data(int clientfd, char *data, ssize_t size)
{
    ssize_t ret, sent = 0;
    while(sent < size){
        ret = Send(clientfd, data + sent, size - sent, 0);
        sent += ret;
    }
}


static void send_response(int clientfd)
{
    send_data(clientfd, header, HSIZE);
    send_data(clientfd, body, BSIZE);
}

static void send_error(int clientfd, const char err[])
{
    HSIZE = strlen(err);
    memcpy(header, err, HSIZE);
    send_data(clientfd, header, HSIZE);
    close(clientfd);
}

static void handle_ping(int clientfd)
{
    BSIZE = strlen(ping_body);
    HSIZE = snprintf(header, HMAX, OK2, BSIZE);
    memcpy(body, ping_body, BSIZE);
    send_response(clientfd);
    close(clientfd);
    
}

static void handle_echo(int clientfd)
{
    char* begin = strstr(request,"\r\n");
    if(begin == NULL){
        send_error(clientfd, bad_400);
        close(clientfd);
        return;
    }
    begin += strlen("\r\n");
    char *end = strstr(begin, "\r\n\r\n");
    if(end == NULL){
        send_error(clientfd, bad_400);
        close(clientfd);
        return;
    }
    // if(end - begin > BMAX +1){
    //     send_error(clientfd, large_413);
    //     close(clientfd);
    //     return;
    // }
    *end = '\0';
    BSIZE = strlen(begin);
    if(BSIZE > BMAX+1){
        send_error(clientfd, large_413);
        close(clientfd);
        return;
    }
    memcpy(body, begin, BSIZE);
    HSIZE = snprintf(header, HMAX, OK2, BSIZE);
    send_response(clientfd);
    close(clientfd);
}

static void handle_read(int clientfd)
{
    HSIZE = snprintf(header, HMAX, OK2, BUF);
    if(BUF > BMAX+1){
        send_error(clientfd, large_413);
        close(clientfd);
        return;
    }
    BSIZE = BUF;
    memcpy(body, buffer, BUF);
    send_response(clientfd);
    close(clientfd);
    
}

static void handle_write(int clientfd)
{
    char* begin = strstr(request,"\r\n\r\n");
    if(begin == NULL){
        send_error(clientfd, bad_400);
        close(clientfd);
        return;
    }
    begin += strlen("\r\n\r\n");
    char* start = strstr(request, "Content-Length: ");
    start += strlen("Content-Length: ");
    char* end = strstr(start, "\r\n");
    if(end == NULL){
        send_error(clientfd, bad_400);
        close(clientfd);
        return;
    }
    *end = '\0';
    BUF = atoi(start);
    if(BUF > BMAX+1){
        send_error(clientfd, large_413);
        close(clientfd);
        return;
    }
    BUF = BUF > BMAX ? BMAX : BUF;
    memcpy(buffer, begin, BUF);
    handle_read(clientfd);
}

static void handle_file(int clientfd)
{
    char* begin = strstr(request, "/");
    assert(begin != NULL);
    begin++;
    begin = strtok(begin, " ");
    int fd = open(begin, O_RDONLY,0);
    if(fd < 0){
        send_error(clientfd, lost_404);
        close(clientfd);
        return;
    }
    struct stat st;
    fstat(fd, &st);
    if(S_ISDIR(st.st_mode)){
        send_error(clientfd, lost_404);
        close(clientfd);
        return;
    }
    const int size = st.st_size;
    HSIZE = snprintf(header, HMAX, OK2, size);
    send_data(clientfd, header, HSIZE);
    while(1){
        RSIZE = read(fd, body, BMAX);
        if(RSIZE == 0){
            break;
        }
        send_data(clientfd, body, RSIZE);
    }
    close(fd);
    close(clientfd);


}

static int accept_client(int listenfd)
{
    struct sockaddr_in client;
    static socklen_t csize = sizeof(client);

    int clientfd = Accept(listenfd, (struct sockaddr *)&client, &csize);
    return clientfd;
}

static void handle_request(int clientfd)
{
    RSIZE = Recv(clientfd, request, RMAX, 0);
    request[RSIZE] = 0;

    if(RSIZE == 0){
        return;
    }
    if(!strncmp(request,ping_req,strlen(ping_req))){
        handle_ping(clientfd);
        return;
    }else if(!strncmp(request,echo_req,strlen(echo_req))){
        handle_echo(clientfd);
        return;
    }else if(!strncmp(request,write_req,strlen(write_req))){
        handle_write(clientfd);
        return;
    }else if (!strncmp(request, read_req, strlen(read_req))){
        handle_read(clientfd);
        return;
    }else if(!strncmp(request, "GET ", strlen("GET "))){
        handle_file(clientfd);
        return;
    }else{
        send_error(clientfd, bad_400);
        return;
    }
    

}
static int open_listenfd(int port)
{
    int listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    static struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    Bind(listenfd, (struct sockaddr *)&server, sizeof(server));
    Listen(listenfd, 10);

    return listenfd;
}

int main(int argc, char * argv[])
{
    int port = get_port();

    printf("Using port %d\n", port);
    printf("PID: %d\n", getpid());

    // Make server available on port
    int listenfd = open_listenfd(port);

 

    // Process client requests
    while(1){
        int clientfd = accept_client(listenfd);
        handle_request(clientfd);
        close(clientfd);
    }

    return 0;
}


static int get_port(void)
{
    int fd = open("port.txt", O_RDONLY);
    if (fd < 0) {
        perror("Could not open port.txt");
        exit(1);
    }

    char buffer[32];
    int r = read(fd, buffer, sizeof(buffer));
    if (r < 0) {
        perror("Could not read port.txt");
        exit(1);
    }

    return atoi(buffer);
}


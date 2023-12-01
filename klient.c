#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char* argv[]){
    int fd, rc;
    char buf[256];
    fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    struct hostent* addrent;
    addrent = gethostbyname(argv[1]);
    memcpy(&addr.sin_addr.s_addr, addrent->h_addr, addrent->h_length);
    connect(fd, (struct sockaddr*) &addr, sizeof(addr));
    rc = read(fd, buf, sizeof(buf));
    write(1, buf, rc);
    close (fd);
}

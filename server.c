#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

struct cln {
    int cfd;
    struct sockaddr_in caddr;
    char nickname[20];
};

void* cthread(void* arg) {
    struct cln* client_info = (struct cln*)arg;
    int cfd = client_info->cfd;
    char buf[256];
    
    ssize_t username_rc = read(cfd, client_info->nickname, sizeof(client_info->nickname) - 1);
    client_info->nickname[username_rc] = '\0';
    printf("Client connected: %s\n", client_info->nickname);
    
    while (1) {
        ssize_t rc = read(cfd, buf, sizeof(buf));
        if (rc <= 0) {
            // Error or connection closed, break from the loop
            break;
        }

        // Null-terminate the received data
        buf[rc] = '\0';

        // Display the received message
        printf("[%lu] Received message from client: %s\n",
               (unsigned long int)pthread_self(), buf);


        // Send the received message back to the client
        write(cfd, buf, strlen(buf));
    }

    // Close the client socket
    close(cfd);
    free(client_info);

    return NULL;
}

int main() {
    pthread_t tid;
    socklen_t sl;
    int sfd;

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(1234);

    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr));
    listen(sfd, 10);

    while (1) {
        int* cfd_ptr = malloc(sizeof(int));
        sl = sizeof(struct sockaddr_in);
        *cfd_ptr = accept(sfd, (struct sockaddr*)&saddr, &sl);

        pthread_create(&tid, NULL, cthread, cfd_ptr);
        pthread_detach(tid);
    }

    // Close the server socket (this part will not be reached in this example)
    close(sfd);

    return 0;
}

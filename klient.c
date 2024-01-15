#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

int fd;
char username[20];
int inRoom = 0;

void* receiveMessages(void* arg) {
    int rc;
    char buf[256];

    while (1) {
        // Read data from the server
        rc = read(fd, buf, sizeof(buf));
        if (rc == -1) {
            perror("Error reading from server");
            break;
        }

        // Check for a specific condition to exit the loop
        if (strcmp(buf, "exit") == 0) {
            break;
        }

        // Null-terminate the received data
        buf[rc] = '\0';

        // Display the received message
        printf("\n%s\n", buf);
    }

    // Exit the thread
    pthread_exit(NULL);
}

void* sendMessages(void* arg) {


    int rc;
    char buf[256];

    while (1) {
        // Get user input for a new message
        printf("\nEnter message to send (or 'exit' to quit): ");
        fgets(buf, sizeof(buf), stdin);

        // Remove newline character from the input
        size_t input_length = strlen(buf);
        if (input_length > 0 && buf[input_length - 1] == '\n') {
            buf[input_length - 1] = '\0';
        }

        // Send the message to the server
        rc = write(fd, buf, strlen(buf));
        if (rc == -1) {
            perror("Error writing to server");
            break;
        }
    }

    // Exit the thread
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        return 1;
    }
    int choice;
    
    printf("Please, provide Your nick (one word only):\n");
    fgets(username, sizeof(username), stdin);
    size_t username_length = strlen(username);


    // Create a socket
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Error creating socket");
        return 1;
    }

    // Set up the server address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    struct hostent* addrent;
    addrent = gethostbyname(argv[1]);
    if (addrent == NULL) {
        perror("Error getting host by name");
        close(fd);
        return 1;
    }

    memcpy(&addr.sin_addr.s_addr, addrent->h_addr, addrent->h_length);

    // Connect to the server
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Error connecting to server");
        close(fd);
        return 1;
    }
    
    // Send the username to the server
    if (write(fd, username, strlen(username)) == -1) {
        perror("Error sending username to server");
        close(fd);
        return 1;
    }    
    printf("Hello %s :)\nYou will currently write to yourself untill you join the room.\nHere are the options You can type:\nshow_users -> shows all logged users\ncreate_room name -> creates a room of the given name\njoin_room name -> joins you to the chosen room\nshow_all_rooms -> shows all rooms\nshow_room_members -> shows all current room members\n", username);

    pthread_t sendThread, receiveThread; 
    // Create threads for sending and receiving messages
    pthread_create(&sendThread, NULL, sendMessages, NULL);
    pthread_create(&receiveThread, NULL, receiveMessages, NULL);

    // Wait for threads to finish
    pthread_join(sendThread, NULL);
    pthread_join(receiveThread, NULL);


    // Close the socket when done
    close(fd);

    return 0;
}

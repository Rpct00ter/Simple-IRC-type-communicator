/*
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
int sendToServer(char* hostname, char* port, char* username){
    printf("Odpalam funkcję\n");
    int fd, rc;
    char buf[256];

    // Create a socket
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Error creating socket");
        return;
    }
    printf("Utworzyłem socket\n");
    // Set up the server address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    struct hostent* addrent;
    addrent = gethostbyname(hostname);
    if (addrent == NULL) {
        perror("Error getting host by name");
        close(fd);
        return;
    }
    printf("Zapisałem adresy serwera\n");

    memcpy(&addr.sin_addr.s_addr, addrent->h_addr, addrent->h_length);
    printf("Wpisałem adres serwera\n");
    // Connect to the server
    connect(fd, (struct sockaddr*) &addr, sizeof(addr));
    printf("Podłączyłem się do serwera\n");
    

    // Main loop for reading and writing until a specific condition
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
        printf("Received: %s\n", buf);

        // Get user input for a new message
        printf("Enter message to send (or 'exit' to quit): ");
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

    // Close the socket when the loop exits
    close(fd);
}


int main(int argc, char* argv[]){
    int choice;
    char username[20];
    printf("Please, provide Your nick(one word only):\n");
    scanf("%s", username);
    while(1){
      printf("Hello %s :)\nPlease, select Your option:\n", username);
      printf("1 -> Log out\n");
      printf("2 -> Show online roomsp\n");
      printf("3 -> Show online users\n");
      printf("4 -> Create room\n");
      printf("5 -> Join room\n");
      printf("6 -> Private message\n");
      printf("7 -> Change your status\n");
      scanf("%d", &choice);
      switch(choice){
        case 1:
          //Exit
          return 0;
        case 2:
          //Show online rooms
          break;
        case 3:
          //Watch online users
          break;
        case 4:
          //Create room
          break;
        case 5:
          //Join room
          break;
        case 6:
          sendToServer(argv[1], argv[2], username);
          break;
        case 7:
          //Change your status (shows during online users lookup)
          break;
        default:
          printf("Invalid choice. Please select a valid option.\n");
          break;
        }
    }
}
*/
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
        printf("Received: %s\n", buf);
    }

    // Exit the thread
    pthread_exit(NULL);
}

void* sendMessages(void* arg) {


    int rc;
    char buf[256];

    while (1) {
        // Get user input for a new message
        printf("Enter message to send (or 'exit' to quit): ");
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

    // Create threads for sending and receiving messages
    pthread_t sendThread, receiveThread;
    pthread_create(&sendThread, NULL, sendMessages, NULL);
    pthread_create(&receiveThread, NULL, receiveMessages, NULL);

    // Wait for threads to finish
    pthread_join(sendThread, NULL);
    pthread_join(receiveThread, NULL);

    // Close the socket when done
    close(fd);

    return 0;
}

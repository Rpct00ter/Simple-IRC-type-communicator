#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#define MAX_USERS 15
#define MAX_USERNAME_LENGTH 20
#define MAX_ROOMS 10
#define MAX_MEMBERS_PER_ROOM 5

struct cln {
    int cfd;
    struct sockaddr_in caddr;
    char nickname[MAX_USERNAME_LENGTH];
    char current_room[MAX_USERNAME_LENGTH]; 
};

struct users {
    size_t counter;
    char usernames[MAX_USERS][MAX_USERNAME_LENGTH];
};
struct users users;

struct room {
    char name[MAX_USERNAME_LENGTH];
    struct cln* members[MAX_MEMBERS_PER_ROOM];
    size_t num_members;
};

struct rooms {
    size_t counter;
    struct room room_list[MAX_ROOMS];
};
struct rooms rooms;

void addUser(struct users* userList, const char* usernames) {
    if (userList->counter < MAX_USERS) {
        strcpy(userList->usernames[userList->counter], usernames);
        userList->counter++;
    } else {
        printf("User list is full. Cannot add more users.\n");
    }
}

// Function to add a room to the global room list
void addRoom(const char* room_name) {
    if (rooms.counter < MAX_ROOMS) {
        strcpy(rooms.room_list[rooms.counter].name, room_name);
        rooms.room_list[rooms.counter].num_members = 0;
        rooms.counter++;
    } else {
        printf("Room list is full. Cannot add more rooms.\n");
    }
}

// Function to add a client to a room
void addClientToRoom(struct cln* client, const char* room_name) {
    for (size_t i = 0; i < rooms.counter; ++i) {
        if (strcmp(rooms.room_list[i].name, room_name) == 0) {
            if (rooms.room_list[i].num_members < MAX_MEMBERS_PER_ROOM) {
                rooms.room_list[i].members[rooms.room_list[i].num_members] = client;
                rooms.room_list[i].num_members++;
                strcpy(client->current_room, room_name);
            } else {
                printf("Room is full. Cannot add more members.\n");
            }
            break;
        }
    }
}

// Function to show all connected users in a room
void showRoomMembers(struct cln* client) {
    printf("Room members in '%s':\n", client->current_room);
    for (size_t i = 0; i < rooms.counter; ++i) {
        if (strcmp(rooms.room_list[i].name, client->current_room) == 0) {
            for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
                printf("- %s\n", rooms.room_list[i].members[j]->nickname);
            }
            break;
        }
    }
}

void showAllUsernames(struct users* userList) {
    printf("All usernames:\n");
    for (size_t i = 0; i < userList->counter; ++i) {
        printf("- %s\n", userList->usernames[i]);
    }
}
   
void showAllRooms() {
    printf("All rooms:\n");
    for (size_t i = 0; i < rooms.counter; ++i) {
        printf("- %s\n", rooms.room_list[i].name);
    }
}

void* cthread(void* arg) {
    struct cln* client_info = (struct cln*)arg;
    int cfd = client_info->cfd;
    char buf[256];
    
    ssize_t username_rc = read(cfd, client_info->nickname, sizeof(client_info->nickname) - 1);
    client_info->nickname[username_rc] = '\0';
    printf("Client connected: %s\n", client_info->nickname);
    addUser(&users, client_info->nickname);
    
    while (1) {
        ssize_t rc = read(cfd, buf, sizeof(buf));
        if (rc <= 0) {
            // Error or connection closed, break from the loop
            break;
        }

        // Null-terminate the received data
        buf[rc] = '\0';
        if (strcmp(buf, "show_users") == 0) {
            // Handle the command to show all usernames
            showAllUsernames(&users);
        } else if (strncmp(buf, "create_room", 11) == 0) {
            // Handle the command to create a room
            char room_name[MAX_USERNAME_LENGTH];
            sscanf(buf, "create_room %s", room_name);
            addRoom(room_name);
            addClientToRoom(client_info, room_name);
        } else if (strncmp(buf, "join_room", 9) == 0) {
            // Handle the command to join a room
            char room_name[MAX_USERNAME_LENGTH];
            sscanf(buf, "join_room %s", room_name);
            addClientToRoom(client_info, room_name);
        } else if (strcmp(buf, "show_room_members") == 0) {
            // Handle the command to show room members
            showRoomMembers(client_info);
        } else if (strcmp(buf, "show_all_rooms") == 0) {
            // Handle the command to show all rooms
            showAllRooms();
        } else {
            // Display and respond to the received message
            printf("[%s] Received message from client: %s\n", client_info->nickname, buf);
            // Send the received message back to the client
            write(cfd, buf, strlen(buf));
        }
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

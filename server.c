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

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;

void addUser(struct users* userList, const char* usernames) {
    pthread_mutex_lock(&users_mutex);
    if (userList->counter < MAX_USERS) {
        strcpy(userList->usernames[userList->counter], usernames);
        userList->counter++;
    } else {
        printf("User list is full. Cannot add more users.\n");
    }
    pthread_mutex_unlock(&users_mutex);
}

// Function to add a room to the global room list
void addRoom(const char* room_name) {
    pthread_mutex_lock(&rooms_mutex);
    if (rooms.counter < MAX_ROOMS) {
        strcpy(rooms.room_list[rooms.counter].name, room_name);
        rooms.room_list[rooms.counter].num_members = 0;
        rooms.counter++;
    } else {
        printf("Room list is full. Cannot add more rooms.\n");
    }
    pthread_mutex_unlock(&rooms_mutex);
}

// Function to add a client to a room
void addClientToRoom(struct cln* client, const char* room_name) {
    pthread_mutex_lock(&rooms_mutex);
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
    pthread_mutex_unlock(&rooms_mutex);
}

void showRoomMembers(struct cln* client) {
    char message[512];
    sprintf(message, "Room members in '%s':\n", client->current_room);
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        if (strcmp(rooms.room_list[i].name, client->current_room) == 0) {
            for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
                sprintf(message + strlen(message), "- %s\n", rooms.room_list[i].members[j]->nickname);
            }
            break;
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    write(client->cfd, message, strlen(message));
}


int isClientInRoom(struct cln* client) {
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
            if (strcmp(rooms.room_list[i].members[j]->nickname, client->nickname) == 0) {
                pthread_mutex_unlock(&rooms_mutex);
                return 1;  // Client is already in a room
            }
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    return 0;  // Client is not in any room
}

void showAllUsernames(struct cln* client) {
    char message[512];
    pthread_mutex_lock(&users_mutex);
    sprintf(message, "All usernames:\n");
    for (size_t i = 0; i < users.counter; ++i) {
        sprintf(message + strlen(message), "- %s\n", users.usernames[i]);
    }
    pthread_mutex_unlock(&users_mutex);
    write(client->cfd, message, strlen(message));
}
   
void showAllRooms(struct cln* client) {
    char message[512];
    pthread_mutex_lock(&rooms_mutex);
    sprintf(message, "All rooms:\n");
    for (size_t i = 0; i < rooms.counter; ++i) {
        sprintf(message + strlen(message), "- %s\n", rooms.room_list[i].name);
    }
    pthread_mutex_unlock(&rooms_mutex);
    write(client->cfd, message, strlen(message));
}

// Function to remove a client from a room
void removeClientFromRoom(struct cln* client) {
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
            if (rooms.room_list[i].members[j] == client) {
                // Shift the remaining members to fill the gap
                for (size_t k = j; k < rooms.room_list[i].num_members - 1; ++k) {
                    rooms.room_list[i].members[k] = rooms.room_list[i].members[k + 1];
                }
                rooms.room_list[i].num_members--;
                client->current_room[0] = '\0';  // Reset the client's current room
                pthread_mutex_unlock(&rooms_mutex);
                return;
            }
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
}

// Function to notify all other room members that a client has left
void notifyRoomMembers(struct cln* client) {
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        if (strcmp(rooms.room_list[i].name, client->current_room) == 0) {
            for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
                int receiver_cfd = rooms.room_list[i].members[j]->cfd;
                if (receiver_cfd != client->cfd) {
                    char leave_message[256];
                    snprintf(leave_message, sizeof(leave_message), "[%s] has left the room.", client->nickname);
                    write(receiver_cfd, leave_message, strlen(leave_message));
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
}

// Function to handle the command to leave a room
void leaveRoom(struct cln* client_info) {
    if (!isClientInRoom(client_info)) {
        // Client is not in any room, inform them
        write(client_info->cfd, "You are not in any room.", sizeof("You are not in any room.") - 1);
    } else {
        // Notify room members that the client has left
        notifyRoomMembers(client_info);
        // Remove the client from the room
        removeClientFromRoom(client_info);
        // Inform the client that they have left the room
        write(client_info->cfd, "You have left the room.", sizeof("You have left the room.") - 1);
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
            showAllUsernames(client_info);
        } else if (strncmp(buf, "create_room", 11) == 0) {
            if (isClientInRoom(client_info)) {
                // Client is already in a room, inform them of their current room
                write(client_info->cfd, "You are already in a room.", sizeof("You are already in a room.") - 1);
            } else {
                // Handle the command to create a room
                char room_name[MAX_USERNAME_LENGTH];
                sscanf(buf, "create_room %s", room_name);
                addRoom(room_name);
                addClientToRoom(client_info, room_name);
                // Send confirmation to the client
                write(client_info->cfd, "Room created successfully.", sizeof("Room created successfully.") - 1);
            }
        } else if (strncmp(buf, "join_room", 9) == 0) {
            if (isClientInRoom(client_info)) {
                // Client is already in a room, inform them of their current room
                write(client_info->cfd, "You are already in a room.", sizeof("You are already in a room.") - 1);
            } else {
                // Handle the command to join a room
                char room_name[MAX_USERNAME_LENGTH];
                sscanf(buf, "join_room %s", room_name);
                addClientToRoom(client_info, room_name);
                // Send confirmation to the client
                write(client_info->cfd, "Joined the room successfully.", sizeof("Joined the room successfully.") - 1);
            }
        } else if (strcmp(buf, "show_room_members") == 0) {
            // Handle the command to show room members
            showRoomMembers(client_info);
        } else if (strcmp(buf, "show_all_rooms") == 0) {
            // Handle the command to show all rooms
            showAllRooms(client_info);
        } else if (strcmp(buf, "leave_room") == 0) {
            leaveRoom(client_info);
        } else {
            // Display and respond to the received message
            printf("[%s] Received message from client: %s\n", client_info->nickname, buf);
            // Send the received message with the sender's name to all clients in the same room
            for (size_t i = 0; i < rooms.counter; ++i) {
                if (strcmp(rooms.room_list[i].name, client_info->current_room) == 0) {
                    for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
                        int receiver_cfd = rooms.room_list[i].members[j]->cfd;
                        if (receiver_cfd != cfd) {  // Exclude the sender
                            // Prepend sender's name to the message
                            char message_with_sender[280];
                            snprintf(message_with_sender, sizeof(message_with_sender), "[%s] %s", client_info->nickname, buf);
                            write(receiver_cfd, message_with_sender, strlen(message_with_sender));
                        }
                    }
                    break;
                }
            }
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

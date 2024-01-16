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

// Struktura przechowująca informacje o kliencie
struct cln {
    int cfd;
    struct sockaddr_in caddr;
    char nickname[MAX_USERNAME_LENGTH];
    char current_room[MAX_USERNAME_LENGTH]; 
};

// Struktura przechowująca informacje o użytkownikach
struct users {
    size_t counter;
    char usernames[MAX_USERS][MAX_USERNAME_LENGTH];
};
struct users users;

// Struktura przechowująca informacje o pokoju
struct room {
    char name[MAX_USERNAME_LENGTH];
    struct cln* members[MAX_MEMBERS_PER_ROOM];
    size_t num_members;
};

// Struktura przechowująca informacje o wszystkich pokojach
struct rooms {
    size_t counter;
    struct room room_list[MAX_ROOMS];
};
struct rooms rooms;

// Mutexy do synchronizacji dostępu do struktur users i rooms
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja dodająca użytkownika do globalnej listy użytkowników
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

// Funkcja dodająca pokój do globalnej listy pokoi
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

// Funkcja dodająca klienta do pokoju
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

// Funkcja wyświetlająca członków pokoju dla danego klienta
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

// Funkcja sprawdzająca, czy klient jest już w pokoju
int isClientInRoom(struct cln* client) {
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
            if (strcmp(rooms.room_list[i].members[j]->nickname, client->nickname) == 0) {
                pthread_mutex_unlock(&rooms_mutex);
                return 1;  // Klient jest już w pokoju
            }
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    return 0;  // Klient nie jest w żadnym pokoju
}

// Funkcja wyświetlająca wszystkie nazwy zalogowanych użytkowników
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

// Funkcja wyświetlająca nazwy wszystkich stworzonych pokoi   
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

// Funkcja usuwająca klienta z pokoju
void removeClientFromRoom(struct cln* client) {
    pthread_mutex_lock(&rooms_mutex);
    for (size_t i = 0; i < rooms.counter; ++i) {
        for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
            if (rooms.room_list[i].members[j] == client) {
                // Przesunięcie pozostałych członków, aby zapełnić lukę
                for (size_t k = j; k < rooms.room_list[i].num_members - 1; ++k) {
                    rooms.room_list[i].members[k] = rooms.room_list[i].members[k + 1];
                }
                rooms.room_list[i].num_members--;
                client->current_room[0] = '\0';  // Zresetowanie aktualnego pokoju klienta
                pthread_mutex_unlock(&rooms_mutex);
                return;
            }
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
}

// Funkcja powiadamiająca wszystkich pozostałych członków pokoju, że klient opuścił pokój
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

// Funkcja obsługująca polecenie opuszczenia pokoju
void leaveRoom(struct cln* client_info) {
    if (!isClientInRoom(client_info)) {
        // Powiadomienie klienta, że nie jest w żadnym pokoju
        write(client_info->cfd, "You are not in any room.", sizeof("You are not in any room.") - 1);
    } else {
        // Powiadom pozostałych członków pokoju, że klient odszedł
        notifyRoomMembers(client_info);
        // Usuń klienta z pokoju
        removeClientFromRoom(client_info);
        // Poinformuj klienta o udanym opuszczeniu pokoju
        write(client_info->cfd, "You have left the room.", sizeof("You have left the room.") - 1);
    }
}

// Wątek obsługujący komunikację z klientem
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
            // Błąd lub zamknięcie połączenia, wyjście z pętli
            break;
        }


        buf[rc] = '\0';
       //Wychodzenie z aplikacji
       if (strcmp(buf, "exit") == 0) {
            leaveRoom(client_info);
            close(cfd);
            //free(client_info);        
        //Wyswietlanie wszystkich użytkownikow na komende "show_users"        
        } else if (strcmp(buf, "show_users") == 0) {
            showAllUsernames(client_info);
        //Tworzenie pokoju na komende "create_room"            
        } else if (strncmp(buf, "create_room", 11) == 0) {
            if (isClientInRoom(client_info)) {
                // Klient jest już w pokoju, wyslanie mu info
                write(client_info->cfd, "You are already in a room.", sizeof("You are already in a room.") - 1);
            } else {
                // Jesli nie ma klienta w pokoju, to moze stworzyc pokoj
                char room_name[MAX_USERNAME_LENGTH];
                sscanf(buf, "create_room %s", room_name);
                addRoom(room_name);
                addClientToRoom(client_info, room_name);
                //Potwierdzenie o utworzeniu pokoju
                write(client_info->cfd, "Room created successfully.", sizeof("Room created successfully.") - 1);
            }
        //Proba dolaczenia do pokoju    
        } else if (strncmp(buf, "join_room", 9) == 0) {
            if (isClientInRoom(client_info)) {
                // Klient jest już w pokoju, wyslanie mu info
                write(client_info->cfd, "You are already in a room.", sizeof("You are already in a room.") - 1);
            } else {
                // Jesli nie ma klienta w pokoju, to mozna go dodac
                char room_name[MAX_USERNAME_LENGTH];
                sscanf(buf, "join_room %s", room_name);
                addClientToRoom(client_info, room_name);
                //Potwierdzenie o dolaczeniu do pokoju
                write(client_info->cfd, "Joined the room successfully.", sizeof("Joined the room successfully.") - 1);
            }
        //Pokazanie klientowi osob w obecnym pokoju    
        } else if (strcmp(buf, "show_room_members") == 0) {
            showRoomMembers(client_info);
        //Pokazanie klientowi wszystkich dostepnych pokoi    
        } else if (strcmp(buf, "show_all_rooms") == 0) {
            showAllRooms(client_info);
        //Proba opuszczenia pokoju przez klienta    
        } else if (strcmp(buf, "leave_room") == 0) {
            leaveRoom(client_info);
        //Jesli nie ma zadnej konkretnej metody to klient wysyla wiadomosci w obszarze pokoju do ktorego nalezy
        } else {
            // Wyswietlenie na serwerze otrzymania wiadomosci
            printf("[%s] Received message from client: %s\n", client_info->nickname, buf);
            //Wysyłanie widaomości w obrębie pokoju
            for (size_t i = 0; i < rooms.counter; ++i) {
                if (strcmp(rooms.room_list[i].name, client_info->current_room) == 0) {
                    for (size_t j = 0; j < rooms.room_list[i].num_members; ++j) {
                        int receiver_cfd = rooms.room_list[i].members[j]->cfd;
                        //Przekazanie w wiadomości informacji o nadawcy
                        char message_with_sender[280];
                        snprintf(message_with_sender, sizeof(message_with_sender), "[%s] %s", client_info->nickname, buf);
                        write(receiver_cfd, message_with_sender, strlen(message_with_sender));
    
                    }
                    break;
                }
            }
        }
    }
    //Zamknięcie socketu klienta
    //close(cfd);
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

    //Zamkniecie socketu servera
    close(sfd);

    return 0;
}

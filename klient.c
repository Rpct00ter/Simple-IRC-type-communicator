#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
int sendToServer(char* argu, char* argv){
  int fd, rc;
  char buf[256];
  fd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv));
  struct hostent* addrent;
  addrent = gethostbyname(argu);
  memcpy(&addr.sin_addr.s_addr, addrent->h_addr, addrent->h_length);
  connect(fd, (struct sockaddr*) &addr, sizeof(addr));
  rc = read(fd, buf, sizeof(buf));
  write(1, buf, rc);
  close (fd);
  return 0;
}

int main(int argc, char* argv[]){
    int choice;
    while(1){
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
          sendToServer(argv[1], argv[2]);
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
          //Text priv (open chat with user)
          break;
        case 7:
          //Change your status (shows during online users lookup)
          break;
        }
    }
}

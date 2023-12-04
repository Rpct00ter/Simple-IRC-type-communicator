#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
#include <sys/select.h>

int main() {
  int sfd, cfd, fdmax, fda, rc, i;
  socklen_t sl;
  struct sockaddr_in saddr, caddr;
  static struct timeval timeout;
  fd_set mask, rmask, wmask;
  //Socket setup
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(1234);
  sfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
  listen(sfd, 10);
  //Select FD sets
  FD_ZERO(&mask);
  FD_ZERO(&rmask);
  FD_ZERO(&wmask);
  fdmax = sfd;
  //Loop
  while (1) {
    FD_SET(sfd, &rmask);
    wmask = mask;
    timeout.tv_sec = 5 * 60;
    timeout.tv_usec = 0;
    rc = select(fdmax + 1, &rmask, &wmask, (fd_set*)0, &timeout);
    if (rc == 0) continue;
    fda = rc;
    // Handling new connections
    if (FD_ISSET(sfd, &rmask)) {
        fda -= 1;
        sl = sizeof(caddr);
        cfd = accept(sfd, (struct sockaddr*)&caddr, &sl);
        FD_SET(cfd, &mask);
        if (cfd > fdmax) fdmax = cfd;
    }
    // Handling existing connections
    for (i = sfd + 1; i <= fdmax && fda > 0; i++) {
        if (FD_ISSET(i, &wmask)) {
            fda -= 1;
            write(i, "Hello World!\n", 13);
            close(i);
            FD_CLR(i, &mask);
            if (i == fdmax)
                while (fdmax > sfd && !FD_ISSET(fdmax, &mask))
                    fdmax -= 1;
        }
    }
  }
  close(sfd);
  return 0;
}

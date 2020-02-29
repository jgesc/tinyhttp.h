#include "tinyhttp.h"

void thttp_send(char * file, struct sockaddr_in client)
{

}

void thttp_serve(int cli)
{
  char buff[1024] = {0};
  recv(cli, buff, 1024, 0);
  printf("RECEIVED:\n\n%s\n", buff);
  send(cli, "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello world!", 56, 0);
  shutdown(cli, 2);
}

void thttp_main(char * root, uint16_t port)
{
  // Open socket
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(8080)
  };
  int sock;
  int opt = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  listen(sock, 0);

  // Listen for connections
  struct sockaddr_in cliaddr = {0};
  socklen_t addrlen;
  int clisock;
  clisock = accept(sock, (struct sockaddr *)&cliaddr, (socklen_t *)&addrlen);

  // Serve request
  thttp_serve(clisock);

  shutdown(sock, 2);
}

int main(void)
{
  thttp_main(NULL, 8080);

  return 0;
}

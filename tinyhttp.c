#include "tinyhttp.h"

const char * RES_200 = "OK";
const char * RES_404 = "Not Found";
const char * RES_403 = "Forbidden";
const char * RES_UND = "Undefined";

const char * thttp_resstr(int code)
{
  switch(code)
  {
    case 200:
      return RES_200;
    case 404:
      return RES_404;
    case 403:
      return RES_403;
    default:
      return RES_UND;
  }
}

int thttp_parse_path(char * in, char * out, size_t n)
{
  if(strstr(in, "..")) return 0;
  int ret;
  if(in[strlen(in) - 1] == '/')
    ret = snprintf(out, n, ".%sindex.html", in);
  else
    ret = snprintf(out, n, ".%s", in);
  return ret > 0 && ret < n;
}

void thttp_sendhdr(int sock, int code)
{
  char buf[1024];
  int l = snprintf(buf, 1024, "HTTP/1.1 %d %s\r\n", code, thttp_resstr(code));
  printf("Sent header '%s'\n", buf);
  send(sock, buf, l, 0);
}

void thttp_handle_get(int cli, char * path)
{
  printf("Trying to access %s\n", path);
  FILE * f = fopen(path, "r");
  if(!f)
  {
    printf("File not found\n");
    thttp_sendhdr(cli, 404);
  }
  else
  {
    thttp_sendhdr(cli, 200);
    fseek(f, 0L, SEEK_END);
    size_t len = ftell(f);
    char obuff[1024];
    len = snprintf(obuff, 1024, "Content-Length: %lu\r\n\r\n", len);
    send(cli, obuff, len, 0);
    rewind(f);
    while(len = fread(obuff, 1, 1024, f))
    {
      send(cli, obuff, len, 0);
    }
  }
  shutdown(cli, 2);
}

int thttp_rcvline(int sock, char * buf, int n)
{
  int cr = 0;
  int off = 0;

  while(recv(sock, buf + off, 1, 0) > 0 && off < n)
  {
    if(buf[off] == '\n')
    {
      if(cr)
      {
        buf[off - 1] = '\0';
        return 1;
      }
      else
      {
        return 0;
      }
    }
    cr = (buf[off] == '\r');

    off++;
  }

  return 0;
}

int thttp_recvwrd(int sock, char * buf, int n)
{
  int cr = 0; // CR flag
  int off = 0; // Offset

  while(recv(sock, buf + off, 1, 0) > 0 && off < n)
  {
    // Check for space
    if(buf[off] == ' ')
    {
      buf[off] = '\0';
      return 1;
    }

    // Check for end of line
    if(buf[off] == '\n')
    {
      if(cr)
      {
        buf[off - 1] = '\0';
        return 1;
      }
      else
      {
        // Invalid line
        return 0;
      }
    }

    // Set CR flag
    cr = (buf[off] == '\r');

    off++;
  }

  // Invalid line
  return 0;
}

enum request thttp_req_parse(int cli)
{
  // Parse request type
  char reqbuf[8];
  thttp_recvwrd(cli, reqbuf, 8);

  if(strcmp(reqbuf, "GET") == 0)
    return GET;
  else
    return INVALID;
}

void thttp_serve(int cli)
{
  char rawpath[512];
  char path[600];

  enum request req = thttp_req_parse(cli);
  thttp_recvwrd(cli, rawpath, 512);
  thttp_parse_path(rawpath, path, 600);

  switch(req)
  {
    case GET:
      printf("GET\n");
      thttp_handle_get(cli, path);
    default:
      break;
  }

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
  while(1)
  {
    struct sockaddr_in cliaddr = {0};
    socklen_t addrlen;
    int clisock;
    clisock = accept(sock, (struct sockaddr *)&cliaddr, (socklen_t *)&addrlen);

    // Serve request
    thttp_serve(clisock);
  }

  shutdown(sock, 2);
}

int main(void)
{
  thttp_main(NULL, 8080);

  return 0;
}

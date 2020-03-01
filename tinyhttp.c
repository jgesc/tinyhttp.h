#include "tinyhttp.h"

// Response texts
const char * RES_200 = "OK";
const char * RES_400 = "Bad Request";
const char * RES_403 = "Forbidden";
const char * RES_404 = "Not Found";
const char * RES_418 = "I'm a teapot";
const char * RES_UND = "Undefined";

char * fsroot;

/// Helper functions
// Get response code string
const char * thttp_resstr(int code)
{
  switch(code)
  {
    case 200:
      return RES_200;
    case 400:
      return RES_400;
    case 403:
      return RES_403;
    case 404:
      return RES_404;
    case 418:
      return RES_404;
    default:
      return RES_UND;
  }
}

// Fetch line
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

// Fetch word
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

// Parse path
int thttp_parse_path(char * in, char * out, size_t n)
{
  if(strstr(in, "..")) return 0;
  int ret;
  if(in[strlen(in) - 1] == '/')
    ret = snprintf(out, n, "%s/.%sindex.html", fsroot, in);
  else
    ret = snprintf(out, n, ".%s", in);
  return ret > 0 && ret < n;
}

// Send header
void thttp_sendhdr(int sock, int code)
{
  char buf[1024];
  int l = snprintf(buf, 1024, "HTTP/1.1 %d %s\r\n", code, thttp_resstr(code));
  printf("Sent header '%s'\n", buf);
  send(sock, buf, l, 0);
}

/// Request handling
// GET
void thttp_handle_get(int cli, char * path)
{
  // Log request
  printf("GET %s\n", path);

  // Try to open file
  FILE * f = fopen(path, "r");
  if(!f)
  {
    // File not found
    printf("404 File not found\n");
    thttp_sendhdr(cli, 404);
  }
  else
  {
    // Send header
    thttp_sendhdr(cli, 200);
    // Find file size
    fseek(f, 0L, SEEK_END);
    size_t len = ftell(f);
    // Build Content-Length string
    char obuff[1024];
    len = snprintf(obuff, 1024, "Content-Length: %lu\r\n\r\n", len);
    send(cli, obuff, len, 0);
    // Send file
    rewind(f);
    while(len = fread(obuff, 1, 1024, f))
    {
      send(cli, obuff, len, 0);
    }
  }
  shutdown(cli, 2);
}

/// Main functions
// Identify request type
enum request thttp_req_parse(int cli)
{
  // Parse request type
  char reqbuf[8];
  thttp_recvwrd(cli, reqbuf, 8);

  // Select request type
  if(strcmp(reqbuf, "GET") == 0)
    return GET;
  else
    return INVALID;
}

// Serve request
void thttp_serve(int cli)
{
  // Path parsing buffer
  char rawpath[512];
  char path[600];

  // Parse request and path
  enum request req = thttp_req_parse(cli);
  if(req == INVALID)
  {
    // Bad request
    thttp_sendhdr(cli, 400);
    shutdown(cli, 2);
    return;
  }
  // Get requested path
  if(!thttp_recvwrd(cli, rawpath, 512))
  {
    // Bad request
    thttp_sendhdr(cli, 400);
    shutdown(cli, 2);
    return;
  }
  // Parse requested path
  if(!thttp_parse_path(rawpath, path, 600))
  {
    // Forbidden
    thttp_sendhdr(cli, 403);
    shutdown(cli, 2);
    return;
  }

  // Select appropiate request type
  switch(req)
  {
    case GET:
      thttp_handle_get(cli, path);
    default:
      break;
  }

  // Finallize connection
  shutdown(cli, 2);
}

// Main loop
void thttp_main(char * root, uint16_t port)
{
  // Set file root
  fsroot = root;

  // Socket configuration
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(8080)
  };
  int sock;
  int opt = 1;

  // Open socket
  if(((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) ||
    (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) ||
    (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) ||
    (listen(sock, 0) < 0))
  {
    perror("Can not open socket");
    return;
  }

  // Listen for connections
  while(1)
  {
    // Accept connection
    struct sockaddr_in cliaddr = {0};
    socklen_t addrlen;
    int clisock;
    clisock = accept(sock, (struct sockaddr *)&cliaddr, (socklen_t *)&addrlen);
    printf("Incoming request");
    if(clisock < -1)
    {
      perror("Can not accept connection");
      continue;
    }

    // Print connection request
    char addrstr[64] = {0};
    getnameinfo((struct sockaddr*)&cliaddr, addrlen, addrstr, 24, NULL, 0, NI_NUMERICHOST);
    printf("Request from [%s]\n", addrstr);

    // Serve request
    thttp_serve(clisock);
  }

  shutdown(sock, 2);
}

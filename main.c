#include "tinyhttp.h"

int main(int argc, char ** argv)
{
  if(argc != 3)
  {
    printf("Usage:\n%s root port\n", argv[0]);
    return 0;
  }

  thttp_main(argv[1], strtol(argv[2], NULL, 10));

  return 0;
}

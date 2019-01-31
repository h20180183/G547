#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


struct num{
  unsigned int value:10;
}random_num;
int main(int argc, char *argv[])
{
  int fd,ret_val;
  int* buffer = malloc(sizeof(int));
  if(argc != 2)
  {
    printf("Insufficient arguements : Mention - (/dev/adxl_x or /dev/adxl_y or /dev/adxl_z)\n");
    return -1;
  }
  fd = open(argv[1],O_RDONLY);
  if(fd == -1)
  {
    printf("File doesn't exist");
    return -1;
  }
  ret_val = read(fd,buffer,4);
  if(ret_val != 4)
  {
    printf("Incomplete read action");
    return -1;
  }
  random_num.value = *buffer;
  printf("%u",random_num.value);
  return 0;
}

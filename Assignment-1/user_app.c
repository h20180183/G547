#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


struct num{
  unsigned int value:10;
}random_num;
int main(int argc, char *argv[])
{
  char* dev_files[3] = {"/dev/adxl_x","/dev/adxl_y","/dev/adxl_z"};
  int fd,ret_val;
  int i =0;
  int* buffer = malloc(sizeof(int)); //Buffer space for 3 adxl_axes is allocated
  if(argc != 2)
  {
    printf("Insufficient arguements : Mention - (/dev/adxl_x or /dev/adxl_y or /dev/adxl_z)\n");
    return -1;
  }
 fd = open(argv[1], O_RDONLY);
  if(fd == -1)
  {
    printf("%s File doesn't exist or permission denied",argv[1]);
    return -1;
  }
  ret_val = read(fd,buffer,4); // Now the buffer contains the values of all the 3 axes.
  if(ret_val != 4)
  {
    printf("Incomplete read action");
    return -1;
  }
 for(i=0;i<=2;i++)
 {
  if(!strcmp(dev_files[i],argv[1]))  //Here the value of the axis that is requested is printed
    break ;
 }
 random_num.value = *buffer;
 printf("%s : %u\n",dev_files[i],random_num.value);
 return 0;
}

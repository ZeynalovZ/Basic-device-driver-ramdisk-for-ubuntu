#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define DEVICE "/dev/magicbox"

int main() {
	int i,fd,offset=0;
	char ch,write_buf[100],read_buf[100];

	fd=open(DEVICE,O_RDWR);
	if(fd==-1) {
		printf("file %s either does not exist or has been locked by another process\n",DEVICE);
          exit(-1);
        }

    while(1) {
 		scanf(" %c", &ch);
       
		switch(ch) {
			case 'w' :
			   printf("Enter data: ");

			   scanf(" %[^\n]",write_buf);
			   pwrite(fd,write_buf,sizeof(write_buf),offset);
			   offset+=sizeof(write_buf);
			   break;
			case 'r' :
			    pread(fd,read_buf,sizeof(read_buf),offset);
			    offset=offset+sizeof(read_buf);
			    printf(" device : %s\n",read_buf);
			    break;
			case 'e' :
			     close(fd);
			     return 0;
			default:
			  printf("command not recognized \n");
			    break;   
		}
	}	

}

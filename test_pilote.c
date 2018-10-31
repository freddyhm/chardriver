#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void){

	int fd;
	char BufOut[6] = "FREDDY";
	char BufIn[6];
	int n, ret;

	fd = open("/dev/myModuleTest", O_RDWR);

	if(fd < 0){
		printf("Erreur d'ouverture = %d\n", fd);
		return -1;
	}else{
		printf("OPEN!!!\n");
	}

	n = 0;

	while (n < 6){
		ret = write(fd, &BufOut[n], 1);
		n++;
	}

	printf("write %s \n",BufOut);

	
	n = 0;
	while (n < 6){
		ret = read(fd, &BufIn[n], 1);
		printf("value read: %c\n", BufIn[n]);
		n++;
	}

	int exit = close(fd);

	printf("%i", exit);
}










/*

int open()

int write(st){
	
	char tmp[8];
	copy_from_user(tmp, buf, count) // petit buffer le plus que possible
	buffer_push(tmp)

	for () // loop to send to ring buffer one at a time
	return count; // return 1 seul character
}



int fd = ope("/dev/serial0", 0_RDWR);

write(fd, "allo", 4); */
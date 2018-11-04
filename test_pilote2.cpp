#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void){

	int fd;
	int ret;
	char BufOut[7] = "FREDDY";

	fd = open("/dev/myModuleTest", O_WRONLY | O_NONBLOCK);

	if(fd < 0){
		printf("Erreur d'ouverture = %d\n", fd);
		return -1;
	}else{
		printf("OPEN!!!\n");
	}

	char m = 'd';
	ret = write(fd, &BufOut[0], 1);
	int exit = close(fd);

	printf("%i", exit);
	
}
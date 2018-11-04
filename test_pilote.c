#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void){

	int fd;
	int fd1;
	char BufOut[7] = "FREDDY";
	char BufIn[25];
	int n, ret;

	fd = open("/dev/myModuleTest", O_RDONLY);

	if(fd < 0){
		printf("Erreur d'ouverture = %d\n", fd);
		return -1;
	}else{
		printf("OPEN!!!\n");
	}

	/*

	if(fd1 < 0){
		printf("Erreur d'ouverture = %d\n", fd1);
		return -1;
	}else{
		printf("OPEN!!!\n");
	}*/


	//sleep(10);



	//n = 0;

	/*

	while (n < 6){
		ret = write(fd, &BufOut[n], 1);
		n++;
	}
	printf("write %s \n",BufOut); */
	
	
//	n = 0;
	//while (n < 6){
		ret = read(fd, &BufIn, 8);
	//	n++;
	//}

	printf("value read: %s\n", BufIn);
	

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
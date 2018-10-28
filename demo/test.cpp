#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <fcntl.h>

int
main()
{
	char 	buf[5000];
	int 	random_fd;	
	
	random_fd = open("/dev/random", O_RDONLY);
	
	for (int i = 0; i < 5000; i++) {
		read(random_fd, buf + i, 1);
	}

	/* Print random bytes */
	for (int i = 0; i < 5000; i++) {
		printf("%c ", buf[i]);
	}
	
	return 0;
}		

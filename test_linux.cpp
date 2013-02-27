#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include "fallocate.h"

int main()
{
	int fl=open("/tmp/testfile", O_RDWR|O_CREAT, 
		0600); //Yeah, it IS octal!
	if (fl==-1)
		return 1;

	int res=fallocate_wrapper(fl, 100000000LL);
	if (res==0)
		printf("Success!\n");
	close(fl);
	return 0;
}

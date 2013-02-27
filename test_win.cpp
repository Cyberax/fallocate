#include <windows.h>
#include <stdio.h>
#include "fallocate.h"

int wmain(int argc, wchar_t** argv[])
{
	prepare_fallocate_wrapper();

	HANDLE hndl=CreateFileW(L"C:\\test.file", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hndl == INVALID_HANDLE_VALUE)
	{
		printf("Can't open c:\\test.file!\n");
		exit(1);
	}
	int res=fallocate_wrapper(hndl, 1000000000LL);
	printf("Err %d\n", res);
	return 0;
}

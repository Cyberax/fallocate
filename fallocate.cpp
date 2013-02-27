#ifdef WIN32
#include <Windows.h>
#include "fallocate.h"

bool prepare_fallocate_wrapper()
{
	HANDLE cur_token;
	TOKEN_PRIVILEGES new_tp;
	LUID luid;          
	if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&cur_token))
		return false;
	if (!LookupPrivilegeValue (NULL, SE_MANAGE_VOLUME_NAME, &luid))
	{
		CloseHandle(cur_token); //I'd have used ON_BLOCK_EXIT, but want to keep dependency count down :)
		return false;
	}	
	memset(&new_tp, 0, sizeof(TOKEN_PRIVILEGES));
	new_tp.PrivilegeCount = 1;
	new_tp.Privileges[0].Luid = luid;
	new_tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges (cur_token, FALSE, &new_tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		CloseHandle (cur_token);
		return false;
	}
	return true;
}

int fallocate_wrapper(handle_t hndl, long long int size_to_reserve)
{	
	if (size_to_reserve <= 0)
		return 0;

	LARGE_INTEGER minus_one = {0}, zero = {0};
	minus_one.QuadPart = -1;

	//Get the current file position
	LARGE_INTEGER old_pos = {0};
	if (!SetFilePointerEx(hndl, zero, &old_pos, FILE_CURRENT))
		return -1;

	//Movie file position to the new end. These calls do NOT result in the actual allocation of
	//new blocks, but they must succeed.
	LARGE_INTEGER new_pos={0};
	new_pos.QuadPart=size_to_reserve;
	if (!SetFilePointerEx(hndl, new_pos, NULL, FILE_END))
		return -1;
	if (!SetEndOfFile(hndl))
		return -1;

	//Try to use the SetFileValidData call
	if (SetFileValidData(hndl, size_to_reserve)!=0)
		return 0; //Success!

	//Bummer. Can't expand the file this way - now try sparse files	
	DWORD temp=0;
	//Mark the file as sparse.
   	if (DeviceIoControl(hndl, FSCTL_SET_SPARSE, NULL, 0, NULL, 0,  &temp, NULL)!=0)
	{				
		FILE_ZERO_DATA_INFORMATION range;
		range.FileOffset = old_pos;
		range.BeyondFinalZero.QuadPart = old_pos.QuadPart+size_to_reserve;
		//Actually set the sparse range.
		if (DeviceIoControl(hndl, FSCTL_SET_ZERO_DATA, &range, sizeof(range), NULL, 0, &temp, NULL))
			return 0; //Done		
	}

	//Everything failed. Cry :( Write one byte at the end to force the actual
	//allocation.
	if (!SetFilePointerEx(hndl, minus_one, NULL, FILE_END))
		return -1;		
	char initializer_buf [1] = {1};
	DWORD written=0;
	if (!WriteFile(hndl, initializer_buf, 1, &written, NULL))
		return -1;

	return 0;
}
#endif //WIN32

#ifdef __gnu_linux__
#include <unistd.h>
#include <sys/syscall.h>
#include "fallocate.h"

bool prepare_fallocate_wrapper()
{
	return true; //No-op on Linux
}

//Linux is fscked up a little bit differently - there's no exported fallocate64
//call in some old glibc versions. So just make a small syscall wrapper that
//can call the syscall directly.
#define fallocate_syscall_num 285
static int fallocate64_raw(int fd, int mode, __off64_t offset, __off64_t len)
{
	return syscall(fallocate_syscall_num, fd, mode, offset, len);
}

int fallocate_wrapper(handle_t hndl, long long int size_to_reserve)
{
	if (size_to_reserve <= 0)
		return 0;

	long long int cur_pos = lseek64(hndl, 0, SEEK_END);
	if (cur_pos==-1)
		return -1;

        if (fallocate64_raw(hndl, 0, cur_pos, size_to_reserve)==0)
		return 0;

	//No such luck :( Use good old lseek64.
	if (lseek64(hndl, cur_pos+size_to_reserve-1, SEEK_SET)==-1)
		return -1;
	//Really force the file write.
	const char *one="O";
	if (write(hndl, one, 1)==-1)
		return -1;

	return 0;
}
	
#endif //__gnu_linux__


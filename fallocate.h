#ifndef _FALLOCATE_H_
#define _FALLOCATE_H_

#pragma once

#ifdef WIN32
typedef HANDLE handle_t;

#else

#ifdef LINUX
typedef int handle_t;
#else
#error "Unknown platform"
#endif

#endif

/**
	Prepare the fallocate wrapper - must be called before the first file you need to set size of is open.
*/
bool prepare_fallocate_wrapper();

/**
	Returns 0 in case of success, -ERRCODE in case of failure. 
	Consult GetLastError() on Windows or errno on Linux for detailed cases.

	Please note, that we DO NOT guarantee that the reserved space will be zero-filled.
*/
int fallocate_wrapper(handle_t hndl, long long int size_to_reserve);

#endif //_FALLOCATE_H_

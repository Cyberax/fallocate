
test_linux: test_linux.cpp fallocate.h fallocate.cpp
	g++ -o test_linux -I. test_linux.cpp fallocate.cpp

#ifndef INCLUDE_SHARED_MEMORY
#define INCLUDE_SHARED_MEMORY 1

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

template <class type>
class memory {
private:
	int id, bytes, key;
	type* address;
public:
	memory(int key, int bytes = sizeof(type)):
		key(key),
		bytes(bytes)
		{
			id = shmget(key, bytes, IPC_CREAT | 0666);
			address = (type*) shmat(id, NULL, 0);
		}
	inline int getId() const {return id;}
	inline int getBytes() const {return bytes;}
	inline int getKey() const {return key;}
	inline void write(type x) const {memcpy(data(), &x, bytes);}
	inline void write(type* x) const {memcpy(data(), x, bytes);}
	inline type* data() const {return address;} // pointer to data
	inline type read() const {return *address;} // actual value of data
};

#endif /* INCLUDE_SHARED_MEMORY */
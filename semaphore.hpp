#ifndef INCLUDE_SEMAPHORE
#define INCLUDE_SEMAPHORE 1

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>



class semaphore {
private:
	int _id, _key;
	static sembuf WAIT[2], SIGNAL[1];

public:
	semaphore(): _id(-1) {}
	semaphore(int key) {this->key(key);}
	void key(int new_key) {
		// set semaphore's key
		_key = new_key;
		_id = semget(_key, 1, IPC_CREAT | 0666);
	}
	inline int id() const {return _id;}
	inline int key() const {return _key;}
	inline int wait() const {return id() == -1 ? -1 : semop(id(), (sembuf*) WAIT, 2);}
	inline int signal() const {return id() == -1 ? -1 : semop(id(), (sembuf*) SIGNAL, 1);}
};

sembuf semaphore::WAIT[2] = {{0, 0, SEM_UNDO}, {0, 1, SEM_UNDO | IPC_NOWAIT}};
sembuf semaphore::SIGNAL[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};

#endif /* __INCLUDE_SEMAPHORE__ */
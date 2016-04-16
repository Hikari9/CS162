#ifndef INCLUDE_SEMAPHORE
#define INCLUDE_SEMAPHORE 1

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

class semaphore {
private:
	int id, key;
	struct sembuf WAIT[2], SIGNAL[1];

public:
	semaphore(int key):
		key(key),
		id(semget(key, 1, IPC_CREAT | 0666)) {
			WAIT[0].sem_num = 0;
    		WAIT[0].sem_op = 0;
    		WAIT[0].sem_flg = SEM_UNDO;
			WAIT[1].sem_num = 0;
			WAIT[1].sem_op = 1;
			WAIT[1].sem_flg = SEM_UNDO | IPC_NOWAIT;
			SIGNAL[0].sem_num = 0;
			SIGNAL[0].sem_op = -1;
			SIGNAL[0].sem_flg = SEM_UNDO | IPC_NOWAIT;
		}
	int getId() const {return id;}
	int getKey() const {return key;}
	int wait() const {return semop(id, (sembuf*) WAIT, sizeof(WAIT) / sizeof(sembuf));}
	int signal() const {return semop(id, (sembuf*) SIGNAL, sizeof(SIGNAL) / sizeof(sembuf));}
};

#endif /* __INCLUDE_SEMAPHORE__ */
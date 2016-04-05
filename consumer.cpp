#include <iostream>  // getline, string
#include <fstream>   // ifstream
#include <cstdio>    // printf, scanf, perror
#include <cstdlib>   // srand, atoi 
#include <ctime>     // time
#include <queue>     // queue
#include "semaphore.hpp"
#include "shared_memory.hpp"

using namespace std;

// assume one producer/consumer
int main(int argc, char* args[]) {
	srand(time(NULL)); // for random

	// check valid parameters
	if (argc < 3) {
		printf("Missing some arguments!\nUsage: %s <textfile> <shared memory size in bytes>", args[0]);
		return 1;
	}

	// get arguments
	char* file = args[1];
	int bytes = atoi(args[2]);
	int sleepTime = argc > 3 ? atoi(args[3]) : 1000;

	// file stream
	ofstream fout(file);

	// concurrency members
	semaphore access(0xACCE55);
	memory<short> feeding(0xFEEEEED);
	memory<char> food(0xF00000D, bytes);

	printf("Preparing for consumption...\n");\
	enum {IDLE, FEEDING, EXIT};

	// have a buffer for reading
	char buffer[bytes + 1];
	buffer[bytes] = '\0';

	// main loop
	while (true) {
		access.wait();
		if (feeding.read() == FEEDING) {
			// there's food for consumer
			memcpy(buffer, food.data(), bytes);
			feeding.write(IDLE);
			access.signal();

			fout << buffer;
			printf("FOOD!!! Eats (%s)\n", buffer);
		}

		else if (feeding.read() == IDLE) {
			access.signal();
			printf("Waiting for producer...\n");
		}

		else { // feeding.read() == EXIT
			feeding.write(IDLE); // make it idle for the next consumer
			access.signal();
			printf("Producer has no more food. Quitting huhu.\n");
			break;
		}
		usleep(sleepTime * 1000);
	}

	return 0;
}
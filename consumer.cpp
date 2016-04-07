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
	memory<int> status(0xFEEEED); // (bytesize << 1) | IS_CONSUMING
	memory<char> food(0xF0000D + bytes, bytes);

	printf("Preparing for consumption...\n");\
	bool first = true;

	// main loop
	while (true) {
		access.wait();

		if (status.read() % 2 == 1) {
			// there's food for consumer
			int buffer_size = status.read() >> 1;
			int bytes_to_write = min(buffer_size, bytes);

			// debug part
			printf("FOOD!!! Eats ");
			cout.write(food.data(), bytes_to_write);
			printf("\n");

			// actual writing
			fout.write(food.data(), bytes_to_write);
			status.write((buffer_size -= bytes_to_write) << 1);
			access.signal();

			first = false;
		}

		else if (!first && !status.read()) {
			status.write(-2);
			access.signal();
			printf("Producer has no more food. Quitting huhu.\n");
			break;
		}

		else {
			access.signal();
			printf("Waiting for producer...\n");
		}

		usleep(sleepTime * 1000);
	}

	return 0;
}
#include <iostream>		// cin, cout
#include <vector>		// resource, vector
#include <deque>		// process_queue

using namespace std;

// data types

struct process;
typedef vector<int> resource;
typedef deque<process> process_queue;

// process class

struct process {
	int id;
	resource held, need;
	process(int m): held(m), need(m) {}
};

// check if resources are available for use

bool available(resource need, resource pool) {
	for (int i = 0; i < need.size(); ++i)
		if (need[i] > pool[i])
			return false;
	return true;
}

/**
 * @brief       performs banker's algorithm for process ordering to avoid deadlocks
 * @details     enqueued implementation of banker's algorithm O(n^2)
 * @param[in]	pro		a queue of processes that want to allocate resources
 * @param[in]	pool	a list of available resources
 * @returns     a list of process IDs indicating the preferred order of process execution
 *              will have a size less than the number of processes if a deadlock occurs
 */

vector<int> banker(process_queue pro, resource pool) {

	vector<int> order;
	int checked = 0; // counter to check if the queued processes have repeated

	while (checked < pro.size()) {

		// try processing the frontmost process
		process current = pro.front();
		pro.pop_front();

		if (available(current.need, pool)) {

			order.push_back(current.id);
			
			// allocate needs, then immediately return (omit code because redundant)
			// pool[i] -= current.need[i];
			// pool[i] += current.need[i];

			// release all held resources
			for (int i = 0; i < pool.size(); ++i)
				pool[i] += current.held[i];

			// reset process count
			checked = 0;

		} else {

			// cannot allocate, repush process to the back
			pro.push_back(current);
			checked++;

		}

	}

	return order;
}

int main() {
	int t, n, m;
	cin >> t;
	while (t--) {
		cin >> n >> m;
		resource pool(n);
		process_queue pro(n, process(m));

		// input
		for (int i = 0; i < m; ++i) cin >> pool[i];
		for (int i = 0; i < n; ++i) pro[i].id = i + 1;
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				cin >> pro[i].held[j];
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				cin >> pro[i].need[j];

		// get ordering from banker's algo
		vector<int> order = banker(pro, pool);

		// SAFE or UNSAFE?
		if (order.size() == n) cout << "SAFE";
		else  /* incomplete */ cout << "UNSAFE";
		
		// output ordering from banker's algo
		for (int i = 0; i < order.size(); ++i)
			cout << (i ? "-" : " ") << order[i];
		cout << endl;
	}
}
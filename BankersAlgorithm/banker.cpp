#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

// data types

struct process;
typedef vector<int> resource;
typedef deque<process> process_queue;

// process class

struct process {
	int id;
	resource has, need;
	process(int m): has(m), need(m) {}
};

// check if resources are available for use

bool available(const resource& need, const resource& pool) {
	for (int i = 0; i < need.size(); ++i)
		if (need[i] > pool[i])
			return false;
	return true;
}

/**
 * @brief       performs banker's algorithm for process ordering to avoid deadlocks
 * @details     enqueued implementation of banker's algorithm O(n^2)
 * @param[in]	pro		a queue of processes that want to allocate resources
 * @param[in]	res		a list of available resources
 * @returns     a list of process IDs indicating the preferred order of process execution
 *              will have a size less than the number of processes if a deadlock occurs
 */

vector<int> banker(process_queue pro, resource res) {
	vector<int> order;
	for (int checked = 0; checked < pro.size(); pro.pop_front()) {
		// get the process in front of the queue
		const process& cur = pro.front();
		if (available(cur.need, res)) {
			// resources are available for current process
			order.push_back(cur.id);
			for (int i = 0; i < res.size(); ++i)
				res[i] += cur.has[i];
			checked = 0; // reset process count
		}
		else {
			// cannot allocate, repush process
			pro.push_back(cur);
			checked++;
		}
	}
	return order;
}

int main() {
	int t, n, m;
	scanf("%d", &t);
	while (t--) {
		scanf("%d%d", &n, &m);
		resource res(n);
		process_queue pro(n, process(m));
		for (int i = 0; i < m; ++i)
			scanf("%d", &res[i]);
		for (int i = 0; i < n; ++i)
			pro[i].id = i + 1;
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				scanf("%d", &pro[i].has[j]);
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				scanf("%d", &pro[i].need[j]);
		// print the ordering of process IDs
		vector<int> order = banker(pro, res);
		printf(order.size() < n ? "UNSAFE" : "SAFE");
		for (int i = 0; i < order.size(); ++i) {
			if (i) printf("-");
			else printf(" ");
			printf("%d", order[i]);
		}
		printf("\n");
	}
}
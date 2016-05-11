#include <iostream>		// cin, cout
#include <queue>		// queue<int>

using namespace std;
const int N = 1000; // max for O(n^2)
int t, n, m;
int need[N][N], held[N][N], pool[N];
int order[N];

// check if resources are available for use
bool available(int id) {
	for (int i = 0; i < m; ++i)
		if (need[id][i] > pool[i])
			return false;
	return true;
}

/**
 * @brief       performs banker's algorithm for process ordering to avoid deadlocks
 * @details     enqueued implementation of banker's algorithm
 * @returns     the number of processes ordered
 *              will have a size less than the number of processes if a deadlock occurs
 */

int banker() {
	int order_size = 0;
	int checked = 0;
	queue<int> q;
	for (int i = 0; i < n; ++i)
		q.push(i);
	while (checked < q.size()) {
		int cur = q.front(); q.pop();
		if (available(cur)) {
			order[order_size++] = cur;
			for (int j = 0; j < m; ++j)
				pool[j] += held[cur][j];
			checked = 0;
		} else {
			q.push(cur);
			checked++;
		}
	}
	return order_size;
}

int main() {
	cin >> t;
	while (t--) {
		cin >> n >> m;
		// input
		for (int i = 0; i < m; ++i) cin >> pool[i];
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				cin >> held[i][j];
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				cin >> need[i][j];
		// get ordering from banker's algo
		int size = banker();
		if (size == n) cout << "SAFE";
		else  /* incomplete */ cout << "UNSAFE";
		
		// output ordering from banker's algo, 1-based
		for (int i = 0; i < size; ++i)
			cout << (i ? "-" : " ") << order[i] + 1;
		cout << endl;
	}
}


/**
 * @brief       performs banker's algorithm for process ordering to avoid deadlocks
 * @details     brute force implementation of banker's algorithm
 * @returns     the number of processes ordered
 *              will have a size less than the number of processes if a deadlock occurs
 *

#include <vector>
int banker_brute() {
	int order_size = 0;
	vector<int> v;
	for (int i = 0; i < n; ++i)
		v.push_back(i);
	bool changed = true;
	while (changed) {
		changed = false;
		for (int i = 0; i < v.size(); ++i) {
			int cur = v[i];
			if (available(cur)) {
				order[order_size++] = cur;
				for (int j = 0; j < m; ++j)
					pool[j] += held[cur][j];
				v.erase(v.begin() + (i--));
				changed = true;
			}
		}
	}
	return order_size;	
}

*/
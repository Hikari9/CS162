import heapq

class PriorityQueue(object):
	
	def __init__(self, init=None, key=lambda x:x):
		self.key = key
		if init:
			self.heap = [(key(job), job) for job in init]
			heapq.heapify(self.heap)
		else:
			self.heap = []
	
	def push(self, job):
		heapq.heappush(self.heap, (self.key(job), job))

	def pop(self):
		return heapq.heappop(self.heap)[1]

	def top(self):
		return self.heap[0][1]

	def __len__(self):
		return len(self.heap)
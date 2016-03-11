class Job(object):
	instance_id = 1
	def __init__(self, arrival, duration, priority, job_id, terminate=True):
		self.arrival = arrival
		self.duration = duration
		self.priority = priority
		self.id = job_id
		self.terminate = terminate
		self.instance_id = Job.instance_id
		Job.instance_id += 1

	def end(self):
		return self.arrival + self.duration
	def __str__(self):
		return '(id=%d arrival=%d, duration=%d terminate=%d)' % (self.id, self.arrival, self.duration, self.terminate)

def first_come_first_serve(job):
	return (job.arrival, job.id)

def shortest_job_first(job):
	return (job.duration + job.arrival, job.duration, job.id)

def shortest_remaining_time_first(job):
	return (job.arrival, job.arrival + job.duration, job.id)

def priority(job):
	return (job.arrival, job.priority, job.id)

def round_robin(job):
	return (job.arrival, job.instance_id)

def pre_emptive(job_key):
	return job_key in [priority, shortest_remaining_time_first]

# prepare the job key mapping
job_keys = {
	'FCFS': first_come_first_serve,
	'SJF':  shortest_job_first,
	'SRTF': shortest_remaining_time_first,
	'P':    priority,
	'RR':   round_robin
}
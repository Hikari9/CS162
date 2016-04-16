class Job(object):

	instance_id = 1
	def __init__(self, arrival, duration, priority, job_id, terminate=True):
		self.arrival = arrival
		self.duration = duration
		self.priority = priority
		self.id = job_id
		self.terminate = terminate
		self.instance_id = 0
		global last_index
		self.idd = last_index[job_id] if job_id in last_index else -1

	'''
	@brief {the constructor}
	
	@param 	self		{Job}
	@param 	arrival 	{int} 	the arrival time of the job
	@param 	duration 	{int} 	the job duration
	@param 	priority 	{int} 	the priority of the job used for priority scheduling
	@param 	job_id 		{int} 	the id of the job
	@param 	terminate 	{bool} 	describes if the job is a terminal task or not
	'''

	def end(self):
		return self.arrival + self.duration

	'''
	@brief {derives the endint time of the job}
	
	@param self {Job}
	@return {int}
	'''

	def __str__(self):
		return '(id=%d arrival=%d, duration=%d terminate=%d)' % (self.id, self.arrival, self.duration, self.terminate)

def first_come_first_serve(job):
	return (job.arrival, job.priority, job.id)

def shortest_job_first(job):
	return (job.arrival, job.duration, job.id)

def shortest_remaining_time_first(job): # same as SJF, but is pre-emptive
	return (job.arrival, job.duration, job.id)

def priority(job):
	return (job.arrival, job.priority, job.id)

last_index = {}
	
def round_robin(job):
	return (job.arrival, job.idd, job.priority, job.id)

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
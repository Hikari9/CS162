from job import Job
from scheduler import get_ready_queue

# input the number of test cases
T = int(raw_input())
for tc in xrange(1, T + 1):

	# input the number of jobs and the pattern string
	line = str(raw_input()).split(' ')
	num_jobs = int(line[0])
	pattern = line[1]
	time_quantum = int(line[2]) if pattern == 'RR' else -1

	# create the job list
	job_list = []

	# add jobs to the list
	for i in xrange(int(num_jobs)):
		A, B, P = map(int, str(raw_input()).split(' '))
		if pattern != 'P':
			P = A #make priority the arrival time for FCFS
		job = Job(
			arrival = A,
			duration = B,
			priority = P,
			job_id = i + 1
		)
		job_list.append(job)
	
	# get the ready queue
	print tc

	ready_queue = get_ready_queue(job_list, pattern, time_quantum)
	for job in ready_queue:
		print '%d %d %d' % (job.arrival, job.id, job.duration) + ('X' if job.terminate else '')
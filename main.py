# input the number of test cases
T = int(raw_input())
for tc in xrange(1, T + 1):

	# input the number of jobs and the pattern string
	line = str(raw_input()).split(' ')
	num_jobs = int(line[0])
	pattern = line[1]
	time_quantum = int(line[2]) if pattern == 'RR' else -1

	# create the job list
	from job import Job
	job_list = []

	# add jobs to the list
	for i in xrange(int(num_jobs)):
		A, B, P = map(int, str(raw_input()).split(' '))
		job = Job(
			arrival = A,
			duration = B,
			priority = P,
			job_id = i + 1
		)
		job_list.append(job)
	
	# get the ready queue
	from scheduler import get_ready_queue
	for job in get_ready_queue(job_list):
		print '%d %d %d' % (job.arrival, job.id, job.duration) + ('X' if job.terminate else '')
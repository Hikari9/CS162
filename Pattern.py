from job import *
from pqueue import PriorityQueue

# this is the main program
if __name__ == '__main__':

	# input the number of test cases
	num_test_cases = int(raw_input())
	for test_case in xrange(1, num_test_cases + 1):

		# input the number of jobs and the pattern string
		line = str(raw_input()).split(' ')
		num_jobs     = int(line[0])
		pattern      = line[1]
		time_quantum = int(line[2]) if pattern == 'RR' else -1

		# obtain job key function later for the queue
		job_key = job_keys[pattern]

		# create the job list
		job_list = []

		# add input to the queue
		for i in xrange(int(num_jobs)):
			A, B, P = map(int, str(raw_input()).split(' '))
			job_list.append(Job(arrival=A, duration=B, priority=P, job_id=i+1))
		
		job_queue = PriorityQueue(job_list, key=job_key)

		ready_queue = []
		time_elapsed = 0

		def schedule(job):

			# schedule a job to the ready queue
			if ready_queue and ready_queue[-1].id == job.id:
				
				# just extend last job
				# this happens when pre-emptive
				ready_queue[-1].duration += job.duration
				ready_queue[-1].terminate = job.terminate
		
			else:
				ready_queue.append(job)

			# update time elapsed after scheduling
			# print 'Scheduled', job
			global time_elapsed
			time_elapsed = ready_queue[-1].end()
		
		# iterate through job queue until it's empty
		while job_queue:

			job = job_queue.pop()
			
			if time_elapsed <= job.arrival:

				# job can be processed
				if job_key != round_robin or job.duration <= time_quantum:

					if not pre_emptive(job_key):
						schedule(job)

					else:
						# look for jobs to interrupt
						put_back = []
						interrupted = False
						while job_queue and job_queue.top().arrival < job.end():
							interrupt = job_queue.top()
							split_job = Job(arrival=interrupt.arrival, duration=job.end()-interrupt.arrival, priority=job.priority, job_id=job.id, terminate=job.terminate)
							
							if job_key(interrupt) < job_key(split_job):
								# job has been interrupted!
								# split then repush current job
								job.duration -= split_job.duration
								job.terminate = False
								job_queue.push(job)

								# push new splitted job
								job_queue.push(split_job)

								# return tested jobs to job queue
								for tested_job in put_back:
									job_queue.push(tested_job)

								interrupted = True
								break

							put_back.append(job_queue.pop())

						if not interrupted:
							# return tested jobs to job queue
							for tested_job in put_back:
								job_queue.push(tested_job)
							# finalize job schedule
							schedule(job)
				else:
					# slice duration time quantum for round robin
					job_slice = Job(arrival=job.arrival+time_quantum, duration=job.duration-time_quantum, priority=job.priority, job_id=job.id, terminate=job.terminate)
					job.duration = time_quantum
					job.terminate = False
					schedule(job)
					job_queue.push(job_slice)

			else:
				# repush to update job arrival time
				job.arrival = time_elapsed
				job_queue.push(job)

		print test_case
		for job in ready_queue:
			print '%d %d %d' % (job.arrival, job.id, job.duration) + ('X' if job.terminate else '')
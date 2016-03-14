from job import *
from pqueue import PriorityQueue

def get_ready_queue(job_list, pattern='FCFS', time_quantum=0):
	
	# obtain job key function as key for priority queue comparison
	job_key = job_keys[pattern]

	# prepare job queue
	job_queue = PriorityQueue(job_list, key=job_key)

	# prepare the ready queue
	ready_queue = []
	global time_elapsed
	time_elapsed = 0

	def schedule(job):
		# schedule a job to the ready queue
		if ready_queue and ready_queue[-1].id == job.id:
			# merge with the previous job because same ID
			# this happens when pre-emptive
			ready_queue[-1].duration += job.duration
			ready_queue[-1].terminate = job.terminate	
		else:
			# append new job
			ready_queue.append(job)
		# update time elapsed after scheduling
		global time_elapsed
		time_elapsed = ready_queue[-1].end()

	# simulate scheduling algorithm
	while job_queue:

		# get the next job
		job = job_queue.pop()
		if job.arrival < time_elapsed: # this job will arrive later, repush to the queue
			job.arrival = time_elapsed
			job_queue.push(job)

		elif job_key == round_robin and job.duration > time_quantum:
			# split job into two for round robin
			part_two = Job(
				arrival   = job.arrival + time_quantum,
				duration  = job.duration - time_quantum,
				priority  = job.priority,
				job_id    = job.id,
				terminate = job.terminate
			)
			job_queue.push(part_two) # repush part 2
			# process part 1
			job.duration = time_quantum
			job.terminate = False
			schedule(job)

		elif not pre_emptive(job_key):
			# schedule immediately
			schedule(job)

		else: # pre_emptive

			# special case: find possible jobs that can interrupt
			put_back = []
			interrupted = False

			while job_queue and job_queue.top().arrival < job.end():
				
				# check if the next job is better than having the other end of the current job
				interrupt = job_queue.top()
				split_job = Job(
					arrival   = interrupt.arrival,
					duration  = job.end() - interrupt.arrival,
					priority  = job.priority,
					job_id    = job.id,
					terminate = job.terminate
				)
				
				if job_key(interrupt) < job_key(split_job):
					# job has been interrupted! split then repush into the queue
					job.duration -= split_job.duration
					job.terminate = False

					# push new parts of the job
					job_queue.push(job)
					job_queue.push(split_job)
					interrupted = True
					break

				put_back.append(job_queue.pop())

			# return tested jobs back into queue
			for tested_job in put_back:
				job_queue.push(tested_job)

			if not interrupted:
				# finalize job schedule
				schedule(job)

	return ready_queue
	

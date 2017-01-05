import os
import sys
import threading, time

asyncQueue = []
threshold = 6
lock = threading.Lock()

def produce():
	print('produce loop thread start')
	while True:
		while len(asyncQueue) < threshold:
			try:
				lock.acquire()
				asyncQueue.append('task')
				print('add a request to queue')
			finally:
				lock.release()		


def callback(task):
	print('content of task = %s' % task)
	return

def consumer():
	print('consumer loop thread start')
	# time.sleep(600)
	counter = 0
	while True:
		time.sleep(6)
		if not len(asyncQueue):
			counter = counter + 1
			if counter >= 3:
				return
			continue
		try:
			lock.acquire()
			task = asyncQueue[0]
			print('consuming queue, current size of queue is %d' % len(asyncQueue))
			callabck(task)
			time.sleep(5)
			asyncQueue.pop(0)
		finally:
			lock.release()	

worker_thread = threading.Thread(target = consumer, name = 'consumerThread')
server_thread = threading.Thread(target = produce, name = 'produceThread')
server_thread.start()
worker_thread.start()
server_thread.join()
worker_thread.join()


		
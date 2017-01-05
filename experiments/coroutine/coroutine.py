import os
import sys


def produce():
	r = ''
	print('switch to await')
	while True:
		res = yield r
		# print type(r)
		if not res:
			continue
		print('get result ==> %d' % res)
		r = '200 OK'


def consumer(gen):
	# initialize generator
	gen.send(None)
	n = -1;
	while n < 5:
		n = n + 1
		# switch to produce for resource
		print('producting resource %s...' % n)
		r = gen.send(n)
		print('consuming resource %s...' % r) 
	gen.close()

gen = produce()
consumer(gen)



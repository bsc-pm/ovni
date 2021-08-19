import numpy as np
import matplotlib.pyplot as plt
import sys

if len(sys.argv) != 2:
	print("missing drift file")
	exit(1)

fn = sys.argv[1]

data = np.genfromtxt(fn, skip_header=1)

with open(fn, 'r') as f:
	lines = [n for n in f.readline().strip().split(" ") if n != '']

node_names = lines[1:]
nnodes=len(node_names)

t = data[:,0]

t -= t[0]

plt.figure(figsize=(10,6))

for i in range(nnodes):
	delta = data[:,i+1]
	delta -= delta[0]

	delta /= 1000

	plt.plot(t, delta, label="rank%d (%s)" % (i, node_names[i]))

plt.title('Clock drift using %d nodes' % nnodes)
plt.xlabel('Relative wall clock time (s)')
plt.ylabel('Relative time delta (us)')
plt.legend()
plt.grid()

#plt.show()
plt.savefig("drift.png")

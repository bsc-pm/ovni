#
# Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

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

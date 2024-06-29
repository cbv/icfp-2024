import subprocess
import re

for a in range(-100,101):
	for b in range(-100,101):
		ret = subprocess.run(f'tail -n +2 ../solutions/threed/threed4-div.txt | ./3v {a} {b}', shell=True, capture_output=True)
		out = ret.stdout.decode('utf8')
		x = re.search(r'Output Written: (-?\d+)', out)
		assert(x)
		assert(int(x[1]) == max(a,b))

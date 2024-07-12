import sys
import serial
import time
from serial.tools import list_ports

comPort = ""
if len(sys.argv) < 2:
	comPort = "COM36"
else:
	comPort = sys.argv[1]

print (comPort)

ports = list(list_ports.comports())
for p in ports:
	print (p)
	 


s = serial.Serial(comPort)
outFile = open("output\output.txt", "wb")

block = 0
errorCount = 0
#15625
while block < 15625 and errorCount < 5:
	cmd = bytes('printblock:' + str(block) + '\n', 'utf-8')
	print(f'Block {block}')
	s.write(cmd)

	lineCount = 0
	serialString = b""			# Used to hold data coming over UART
	start = time.time()
	complete = 0

	while time.time() - start < 2:
		bytesWaiting = s.inWaiting()
		if (bytesWaiting != 0):
			serialString += s.read(bytesWaiting)
			if serialString.count(b'\n') > 256:
				complete = 1
				break

	
	if not complete:
		print("Incomplete")
		errorCount += 1
		s.reset_input_buffer()
	else:
		block += 1
		errorCount = 0
		outFile.write(serialString.strip(b'\r'))
	
exit()
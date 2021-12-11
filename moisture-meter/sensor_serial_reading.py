import serial # pip install pyserial

ser = serial.Serial(
    port='COM3',\
    baudrate=9600,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout=0)

print("connected to: " + ser.portstr)

for i in range(0,10): # number of reads 
    while ser.inWaiting()<5: # depends on number of chars in output string
        pass
    print(chr(ser.readline()[1])) # how to extract str out of char
ser.close()
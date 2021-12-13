import serial # pip install pyserial
YELLOW_DESCR = "yellow"
GREEN_DESCR = "green"

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
    #print(type(chr(ser.readline()[0]))) # how to extract str out of char
    input_str = ser.readline()
    print(f"{YELLOW_DESCR}:{chr(input_str[0])}, {GREEN_DESCR}:{chr(input_str[2])}")
ser.close()
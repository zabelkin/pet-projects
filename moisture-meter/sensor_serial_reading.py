import serial # pip install pyserial
import smtplib
from email.message import EmailMessage

# names for flowers
YELLOW_DESCR = "жёлтый"
GREEN_DESCR = "зелёный"

ser = serial.Serial(
    port='COM3',\
    baudrate=9600,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout=0)

print("connected to: " + ser.portstr)
yellow_lst = []
green_lst = []

for i in range(0,10): # number of reads 
    while ser.inWaiting()<3: # wait for serial input, depending on number of chars in output string
        pass
    input_str = ser.readline()
    yellow_lst.append(int(chr(input_str[0]))) # str out of char
    green_lst.append(int(chr(input_str[2])))
    #print(f"{YELLOW_DESCR}:{chr(input_str[0])}, {GREEN_DESCR}:{chr(input_str[2])}")
ser.close()

yellow_avg = int(sum(yellow_lst)/len(yellow_lst))
green_avg = int(sum(green_lst)/len(green_lst))

print(f"yellow in avg = {yellow_avg}")
print(f"green in avg = {green_avg}")

msg = EmailMessage()
msg.set_content(f"{YELLOW_DESCR} = {yellow_avg}, {GREEN_DESCR} = {green_avg}") 
msg['From'] = 'zabelkin@tdme.ru'
msg['To'] = 'zabelkin@tdme.ru, dunina@tdme.ru'
#msg['To'] = 'zabelkin@tdme.ru'
msg['Subject'] = 'замер влажности почвы'

s = smtplib.SMTP('mail.tdme.ru')
s.send_message(msg)
s.quit()


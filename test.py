import serial
ser = serial.Serial("/dev/ttyUSB0", 921600, timeout=1)
while True:
    d = ser.read(100)
    print(len(d))

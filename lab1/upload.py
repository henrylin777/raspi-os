import serial
import os
import sys


def main(fname:str):
    serialport = serial.Serial("COM3", baudrate=115200)
    fsize = os.stat(fname).st_size.to_bytes(4, "little")
    serialport.write(fsize)
    with open(fname,"rb") as f:
        serialport.write(f.read())


if __name__ == "__main__":
    img_file = sys.argv[1]
    main(img_file)

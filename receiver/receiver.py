# https://stackoverflow.com/questions/676172/full-examples-of-using-pyserial-package
# Must close serial monitor...
import serial
import struct
import time
import msvcrt
# from pathlib import Path
# import os

# Initializations
retries_max = 3

# Obj instantiation
ser = serial.Serial(port="COM3", baudrate=921600, timeout=5)  # Change port accordingly
# Resets
ser.reset_input_buffer()
ser.reset_output_buffer()
print("Connected!")

ser.isOpen()
print("Serial connection is now open, press 'Q' to quit.")

while True:
    # Check if 'Q' is pressed, this can also be an interrupt
    # input_kb = input()
    # if input_kb == 'Q': 
    #     print("Closing...")
    #     ser.close()
    #     exit()  # 
    #     break
    if msvcrt.kbhit():  # Non-blocking
        if msvcrt.getch() in (b'q', b'Q'):
            print("Closing...")
            ser.close()
            break

    # Sequence- get header from sendSize (header size is 4 since uint32_t), get contents from sendImg
    # Note- must have protocol for size&sequence of the received data (since we're transmitting sequentially...)
        # Magic # (to implement later...)- look for this to determine start of msgs...
    print("Waiting for img...")

    # line = ser.readline().decode(errors="ignore").strip()  # 
    # if not line.startswith("Size:"):
    #     continue
    # size = int(line.split()[1])

    # Failure at header will count as attempt1..., can also implement it s.t. 
    success = False
    transmission_started = False
    for i in range(retries_max): 
        # Read header
        header = ser.read(4)
        # Idle
        if len(header) == 0:
            break
        transmission_started = True
        # Verify img
        if len(header) != 4: 
            print(f"Error: Received {len(header)}/4 header bytes, retrying (Attempt {i+1}/3)...")
            ser.write(b'N')  # Expects bytes not str's, NACK
            ser.flush()  #
            ser.reset_input_buffer()  # Clear buffer of any leftover bytes
            continue
        else: 
            size = struct.unpack("<I", header)[0]  # < (little endian), I (uint32_t)
            # print(header)
            # print(header.hex())
            print(f"Received img size: {size} bytes")

        # Read img
        image = ser.read(size)
        # Verify img
        if len(image) != size:
            print(f"Error: Received {len(image)}/{size} img bytes, retrying (Attempt {i+1}/3)...")
            ser.write(b'N')
            ser.flush()
            ser.reset_input_buffer()
            continue
        else:
            ser.write(b'A')  # ACK

            # Save img
            with open(f"imgs/IMG_{time.time_ns()}.jpg", "wb") as f:
                f.write(image)
            success = True
            print("Image saved")
            break
    
    if transmission_started and not success:
        print("Error: retried transmission 3 times but still failed, proceeding...")
import serial
import serial.tools.list_ports
import math
from vpython import *

# 1. Find the Seeed Studio XIAO ESP32-C3 port
def get_xiao_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if port.vid == 0x303A or "usb serial device" in port.description.lower():
            return port.device
    return None

port_name = get_xiao_port()

if not port_name:
    print("XIAO ESP32-C3 not found! Check your cable connection.")
    exit()

print(f"Connecting to {port_name}...")
ser = serial.Serial(port_name, 115200, timeout=1)

# 2. Set up the 3D scene and aircraft model
scene.range = 5
scene.background = color.gray(0.2)
plane = box(length=4, width=3, height=0.5, color=color.cyan)

# 3. Main real-time loop
while True:
    rate(50)  # Render update rate
    
    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue
                
            pitch_deg, roll_deg, yaw_deg = map(float, line.split(','))
            
            pitch = math.radians(pitch_deg)
            roll = math.radians(roll_deg)
            yaw = math.radians(yaw_deg)
            
            # Reset orientation before applying new angles
            plane.axis = vector(1, 0, 0)
            plane.up = vector(0, 1, 0)
            
            plane.rotate(angle=yaw, axis=vector(0, 1, 0))
            plane.rotate(angle=pitch, axis=vector(0, 0, 1))
            plane.rotate(angle=roll, axis=vector(1, 0, 0))
            
        except (ValueError, UnicodeDecodeError):
            # Skips incomplete lines during initial connection
            pass
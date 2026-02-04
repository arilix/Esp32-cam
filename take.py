import serial
import cv2
import numpy as np
import struct
import os
import time

PORT = "/dev/ttyUSB0"
BAUD = 2000000

ser = serial.Serial(PORT, BAUD, timeout=1)

cv2.namedWindow("ESP32-CAM USB", cv2.WINDOW_NORMAL)

SAVE_DIR = os.path.expanduser("~/Documents/ESP32_CAM")
os.makedirs(SAVE_DIR, exist_ok=True)

print("Streaming... Press S to save, Q to quit")

while True:
    # === WAIT HEADER ===
    header = ser.read(5)
    if header != b"FRAME":
        continue

    # === READ SIZE ===
    size_bytes = ser.read(4)
    if len(size_bytes) < 4:
        continue

    frame_size = struct.unpack("<I", size_bytes)[0]

    # === READ JPEG ===
    jpg = ser.read(frame_size)
    if len(jpg) != frame_size:
        continue

    img = cv2.imdecode(np.frombuffer(jpg, np.uint8), cv2.IMREAD_COLOR)
    if img is None:
        continue

    h, w, _ = img.shape
    cx, cy = w // 2, h // 2

    # === CROSSHAIR ===
    cv2.line(img, (cx-20, cy), (cx+20, cy), (0,255,0), 2)
    cv2.line(img, (cx, cy-20), (cx, cy+20), (0,255,0), 2)

    cv2.putText(img, "CAMERA LIVE",
                (10,30), cv2.FONT_HERSHEY_SIMPLEX,
                1, (0,255,0), 2)

    cv2.imshow("ESP32-CAM USB", img)

    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('s'):
        path = f"{SAVE_DIR}/img_{int(time.time())}.jpg"
        cv2.imwrite(path, img)
        print("Saved:", path)

ser.close()
cv2.destroyAllWindows()

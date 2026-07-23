import serial
import time

PORT = "COM3"   # USB-RS485 adapter COM port
BAUD = 38400

FRAME_TYPES = {
    0: "Token",
    1: "Poll For Master",
    2: "Reply To PFM",
    3: "Test Request",
    4: "Test Response",
    5: "BACnet Data Expecting Reply",
    6: "BACnet Data Not Expecting Reply",
    7: "Reply Postponed",
}

ser = serial.Serial(
    PORT,
    BAUD,
    bytesize=8,
    parity="N",
    stopbits=1,
    timeout=0.05,
)

buf = bytearray()

print(f"Listening on {PORT} at {BAUD} baud. Do not transmit anything.")
print("Filtered view: MAC16 / MAC17 / MAC32 / Poll-For-Master\n")

while True:
    data = ser.read(512)
    if data:
        buf.extend(data)

    while True:
        idx = buf.find(b"\x55\xff")
        if idx < 0:
            if len(buf) > 2048:
                buf.clear()
            break

        if idx > 0:
            del buf[:idx]

        if len(buf) < 8:
            break

        frame_type = buf[2]
        dst = buf[3]
        src = buf[4]
        data_len = (buf[5] << 8) | buf[6]

        total_len = 8 + data_len
        if data_len > 0:
            total_len += 2  # data CRC

        if len(buf) < total_len:
            break

        frame = bytes(buf[:total_len])
        del buf[:total_len]

        # Focused filter for this MAC17 test
        interesting = (
            (src == 16 and dst in (17, 18, 32))
            or (src == 17 or dst == 17)
            or (src == 32 and dst in (0, 16))
            or frame_type == 1
        )

        if not interesting:
            continue

        ts = time.strftime("%H:%M:%S")
        frame_name = FRAME_TYPES.get(frame_type, f"Unknown {frame_type}")
        raw = " ".join(f"{b:02X}" for b in frame)

        print(
            f"{ts} type={frame_type} {frame_name:<28} "
            f"src={src:3d} dst={dst:3d} len={data_len:3d} raw={raw}"
        )
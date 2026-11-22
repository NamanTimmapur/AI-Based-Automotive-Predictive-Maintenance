
import can
import random
import csv
import argparse
import time
from datetime import datetime

# Select sensor operating mode
parser = argparse.ArgumentParser()
parser.add_argument(
    "--mode",
    choices=["normal", "high_vibration", "low_oil"],
    default="normal"
)
args = parser.parse_args()

mode = args.mode

# Connect to virtual CAN bus
bus = can.interface.Bus(
    channel="vcan0",
    interface="socketcan"
)

print(f"Sensor ECU Started | Mode: {mode}")

# CSV logging
with open("sensor_data.csv", "a", newline="") as file:
    writer = csv.writer(file)

    if file.tell() == 0:
        writer.writerow([
            "timestamp",
            "mode",
            "vibration_mm_s",
            "oil_pressure_bar"
        ])

    try:
        while True:

            # Generate simulated sensor values
            if mode == "normal":
                vibration = random.uniform(1.0, 3.0)
                oil_pressure = random.uniform(2.5, 4.5)

            elif mode == "high_vibration":
                vibration = random.uniform(7.0, 12.0)
                oil_pressure = random.uniform(2.5, 4.5)

            elif mode == "low_oil":
                vibration = random.uniform(1.0, 3.0)
                oil_pressure = random.uniform(0.5, 1.5)

            # Scale values by 10 to transmit integers
            vibration_raw = int(vibration * 10)
            oil_raw = int(oil_pressure * 10)

            # Encode both readings into 4 bytes
            data = (
                vibration_raw.to_bytes(2, "big")
                + oil_raw.to_bytes(2, "big")
            )

            # Create CAN message
            message = can.Message(
                arbitration_id=0x200,
                data=data,
                is_extended_id=False
            )

            bus.send(message)

            print("--------------------------------")
            print(f"Vibration    : {vibration:.1f} mm/s")
            print(f"Oil Pressure : {oil_pressure:.1f} bar")
            print(f"CAN ID       : 0x200")

            # Save readings
            writer.writerow([
                datetime.now().isoformat(timespec="seconds"),
                mode,
                f"{vibration:.1f}",
                f"{oil_pressure:.1f}"
            ])
            file.flush()

            time.sleep(1)

    except KeyboardInterrupt:
        print("\nSensor ECU stopped safely.")

    finally:
        bus.shutdown()
        print("CAN connection closed.")

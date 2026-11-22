
import can
import time
import random
import csv
import argparse
from datetime import datetime
from pathlib import Path

parser = argparse.ArgumentParser()

parser.add_argument(
    "--mode",
    choices=["normal", "high_load", "overheat"],
    default="normal"
)

args = parser.parse_args()
mode = args.mode

bus = can.interface.Bus(
    channel="vcan0",
    interface="socketcan"
)

print(f"Engine ECU Started | Mode: {mode}")

csv_file = Path("engine_data.csv")
file_exists = csv_file.exists() and csv_file.stat().st_size > 0

try:
    with open(csv_file, "a", newline="") as file:

        writer = csv.writer(file)

        if not file_exists:
            writer.writerow([
                "timestamp",
                "mode",
                "rpm",
                "temperature"
            ])

        while True:

            if mode == "normal":
                rpm = random.randint(800, 3000)
                temperature = random.randint(70, 95)

            elif mode == "high_load":
                rpm = random.randint(2500, 4000)
                temperature = random.randint(90, 105)

            else:
                rpm = random.randint(1500, 3500)
                temperature = random.randint(106, 120)

            # Encode RPM into 2 bytes
            rpm_data = rpm.to_bytes(2, "big")

            # Encode temperature into 1 byte
            temp_data = temperature.to_bytes(1, "big")

            data = rpm_data + temp_data

            message = can.Message(
                arbitration_id=0x100,
                data=data,
                is_extended_id=False
            )

            bus.send(message)

            timestamp = datetime.now().isoformat(
                timespec="seconds"
            )

            writer.writerow([
                timestamp,
                mode,
                rpm,
                temperature
            ])

            file.flush()

            print(
                f"RPM: {rpm} | "
                f"Temperature: {temperature} °C | "
                f"Mode: {mode}"
            )

            time.sleep(1)

except KeyboardInterrupt:
    print("\nEngine ECU stopped safely.")

finally:
    bus.shutdown()
    print("CAN connection closed.")


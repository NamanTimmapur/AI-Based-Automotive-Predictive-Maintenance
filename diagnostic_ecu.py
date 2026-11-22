
import can
import csv
from datetime import datetime
from pathlib import Path

# Connect to virtual CAN bus
bus = can.interface.Bus(
    channel="vcan0",
    interface="socketcan"
)

print("Diagnostic ECU Started...")
print("Waiting for Engine ECU messages...\n")

# Prepare diagnostic log
log_file = Path("diagnostic_data.csv")

file_exists = (
    log_file.exists() and log_file.stat().st_size > 0
)

try:
    with open(log_file, "a", newline="") as file:

        writer = csv.writer(file)

        if not file_exists:
            writer.writerow([
                "timestamp",
                "rpm",
                "temperature",
                "status"
            ])

        while True:

# Process Sensor ECU messages (CAN ID 0x200)
if message.arbitration_id == 0x200:

    if len(message.data) != 4:
        continue

    # Decode vibration
    vibration_raw = int.from_bytes(
        message.data[0:2],
        byteorder="big"
    )

    vibration = vibration_raw / 10.0

    # Decode oil pressure
    oil_raw = int.from_bytes(
        message.data[2:4],
        byteorder="big"
    )

    oil_pressure = oil_raw / 10.0

    # Classify sensor condition
    if vibration >= 7.0:
        status = "HIGH VIBRATION"
        status_code = 1

    elif oil_pressure <= 1.5:
        status = "LOW OIL PRESSURE"
        status_code = 2

    else:
        status = "NORMAL"
        status_code = 0

    print("--------------------------------")
    print(f"Vibration      : {vibration:.1f} mm/s")
    print(f"Oil Pressure   : {oil_pressure:.1f} bar")
    print(f"Sensor Status  : {status}")

    # Send sensor diagnostic result
    diagnostic_message = can.Message(
        arbitration_id=0x301,
        data=[status_code],
        is_extended_id=False
    )

    bus.send(diagnostic_message)

    print("Diagnostic CAN ID: 0x301")
    print(f"Status Code      : {status_code}")

    continue


# Accept Engine ECU messages only
if message.arbitration_id != 0x100:
    continue

            # Receive CAN message
            message = bus.recv(timeout=2)

            if message is None:
                continue

            # Accept only Engine ECU messages
            if message.arbitration_id != 0x100:
                continue

            # Decode RPM
            rpm = int.from_bytes(
                message.data[0:2],
                byteorder="big"
            )

            # Decode temperature
            temperature = message.data[2]

            # Classify engine condition
            if temperature >= 105:
                status = "OVERHEATING"
                status_code = 2

            elif temperature >= 100:
                status = "HIGH TEMPERATURE"
                status_code = 1

            else:
                status = "NORMAL"
                status_code = 0

            print("------------------------------")
            print(f"Engine RPM       : {rpm}")
            print(f"Temperature      : {temperature} C")
            print(f"Engine Status    : {status}")

            # Send diagnostic status over CAN
            diagnostic_message = can.Message(
                arbitration_id=0x300,
                data=[status_code],
                is_extended_id=False
            )

            bus.send(diagnostic_message)

            print(f"Diagnostic CAN ID: 0x300")
            print(f"Status Code      : {status_code}")

            # Save diagnostic result
            writer.writerow([
                datetime.now().isoformat(
                    timespec="seconds"
                ),
                rpm,
                temperature,
                status
            ])

            file.flush()

except KeyboardInterrupt:
    print("\nDiagnostic ECU stopped safely.")

finally:
    bus.shutdown()
    print("Diagnostic CAN connection closed.")

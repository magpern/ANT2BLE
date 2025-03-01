import time
import serial
import logging
from logging.handlers import RotatingFileHandler
from openant.easy.node import Node

# ✅ Set your ANT+ USB device port
ANT_DEVICE_PORT = "/dev/ttyACM0"  # Linux/macOS
# ANT_DEVICE_PORT = "COM3"  # Windows

def reset_ant_usb():
    try:
        print(f"Resetting ANT+ USB device at {ANT_DEVICE_PORT}...")
        ser = serial.Serial(ANT_DEVICE_PORT, 115200, timeout=1)
        ser.setDTR(False)  # ✅ Reset USB device
        time.sleep(1)  # ✅ Wait for reset
        ser.setDTR(True)
        ser.close()
        print("✅ ANT+ USB device reset successfully!")
    except Exception as e:
        print(f"⚠️ Failed to reset ANT+ USB device: {e}")

reset_ant_usb()

ANT_NETWORK_KEY = [0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45]

# ✅ Open /dev/ttyACM0 for writing
serial_out = open(ANT_DEVICE_PORT, "wb", buffering=0)

# ✅ Setup Rolling Log Handler
log_formatter = logging.Formatter('%(asctime)s - %(message)s')
log_handler = RotatingFileHandler("ant_data.log", maxBytes=5 * 1024 * 1024, backupCount=5)  # 5MB per file, 5 backups
log_handler.setFormatter(log_formatter)
log_handler.setLevel(logging.INFO)

logger = logging.getLogger("ANT+Logger")
logger.setLevel(logging.INFO)
logger.addHandler(log_handler)

def process_ant_data(data, device_num, device_type):
    """Processes ANT+ data and logs it, adding device type after 0xA4"""
    message_length = len(data)  # No Message ID included

    # Compute Checksum (XOR of Length, Device Type, and Data)
    checksum = message_length ^ device_type
    for byte in data:
        checksum ^= byte

    # Construct Full ANT+ Message (0xA4 + Device Type + Length + Data + Checksum)
    full_message = bytes([0xA4, device_type, message_length]) + bytes(data) + bytes([checksum])

    # Create escaped hex representation for logging
    escaped_output = "".join(f"\\x{byte:02X}" for byte in full_message)

    log_message = f"📡 Device {device_num} (Type {device_type}): {full_message.hex().upper()} (printf \"{escaped_output}\" > /dev/ttyACM0)"
    
    # ✅ Print to console
    print(log_message)

    # ✅ Log to file
    logger.info(log_message)

    # ✅ Send the raw bytes directly (without reopening the file every time)
    try:
        serial_out.write(full_message)  # Write directly to the serial port
        serial_out.flush()  # Ensure immediate transmission
    except Exception as e:
        print(f"⚠️ Error writing to /dev/ttyACM0: {e}")

# ✅ Data handlers for two separate devices
def on_data_1(data):
    process_ant_data(data, 1, 17)  # Device type 17 (Fitness Equipment)

def on_data_2(data):
    process_ant_data(data, 2, 11)  # Device type 11 (Heart Rate Monitor)

# ✅ Ensure serial is closed on exit
import atexit
atexit.register(serial_out.close)

# ✅ Create ANT+ Node
ant_node = Node()
ant_node.set_network_key(0, ANT_NETWORK_KEY)

# ✅ Create Channel for Second Device
channel2 = ant_node.new_channel(1)
channel2.set_search_timeout(255)
channel2.set_period(8182)
channel2.set_rf_freq(57)
channel2.set_id(0, 11, 5)  # Device type 11 (Heart Rate Monitor)
channel2.on_broadcast_data = on_data_2
channel2.open()

# ✅ Start Listening
try:
    print("Listening for ANT+ data from two devices...")
    ant_node.start()
except KeyboardInterrupt:
    print("Stopping ANT+ Listener...")
    ant_node.stop()
    serial_out.close()

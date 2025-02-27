# ESP32-S3 ANT+ to BLE FTMS Bridge

## 📌 Overview

This project implements an **ESP32-S3-based bridge** that receives **ANT+ sensor data** (Speed, Cadence, Power) and transmits it over **BLE FTMS** (Fitness Machine Service). It is designed to interface with fitness applications like **Zwift, TrainerRoad, and Rouvy**.

The **Raspberry Pi** captures **raw ANT+ data** via USB and forwards it to the ESP32-S3 over **USB Serial**. The ESP32-S3 parses the ANT+ messages and broadcasts them over BLE.

## 🎯 Features

✅ **ANT+ to BLE FTMS bridge** – Converts ANT+ sensor data to BLE FTMS format  
✅ **Supports Speed, Cadence, and Power Sensors** – Reads ANT+ messages and forwards as BLE  
✅ **Modular Code** – Expandable to support additional ANT+ profiles  
✅ **Configurable BLE Device Name** – Set via structured Serial command  
✅ **CRC Validation** – Ensures error-free data transmission  
✅ **Power-Efficient** – Can be optimized for low-power modes  

## 🔧 Hardware Requirements

- **ESP32-S3 Dev Board** (e.g., Waveshare ESP32-S3 Mini)  
- **Raspberry Pi 5** (or similar Linux device for ANT+ capture)  
- **ANT+ USB Stick** (Garmin, Dynastream, or other compatible dongle)  
- **FTMS-compatible BLE Device** (Zwift, TrainerRoad, etc.)  
- **USB-Serial Adapter** (for debugging via GPIO 9/10)  

## ⚙️ Software Requirements

- **PlatformIO** (for ESP32-S3 development)  
- **Python** (for computing CRC and sending structured commands)  
- **nRF Connect App** (for BLE debugging)  

## 🚀 Installation & Setup

### **1️⃣ Install PlatformIO & Build the Firmware**

1. Install **PlatformIO** on your development machine:

   ```sh
   pip install platformio
   ```

2. Clone this repository and build the firmware:

   ```sh
   git clone https://github.com/your-repo/esp32s3-ant-ble-bridge.git
   cd esp32s3-ant-ble-bridge
   pio run -e esp32s3
   ```

3. Flash to ESP32-S3:

   ```sh
   pio run -e esp32s3 -t upload
   ```

### **2️⃣ Configure BLE Device Name (Serial Command)**

Set a custom BLE name using **structured ANT+ style serial command**:

```sh
printf "\xA4\x13\xF0SETNAME MyTrainer\xXX" > /dev/ttyACM0
```

- `0xA4` → Sync Byte  
- `0x13` → Payload Length (including CRC)  
- `0xF0` → Custom Command Flag  
- `"SETNAME MyTrainer"` → BLE Name  
- `0xXX` → CRC (computed via XOR)  

### **3️⃣ Connect to BLE Device**

- Open **nRF Connect** (or Zwift, TrainerRoad)  
- Scan for **"MyTrainer"**  
- Connect and verify that FTMS data is being received  

## 🔥 Serial Command Format

| **Byte** | **Value** | **Description** |
|----------|----------|----------------|
| `0xA4`  | Sync Byte | ANT+ Style Sync |
| `0xXX`  | Length | Total Payload Size (Includes CRC) |
| `0xF0`  | Command Flag | Custom Serial Command |
| Payload | ASCII Text | e.g., `"SETNAME MyTrainer"` |
| `0xXX`  | CRC | XOR Checksum |

## 🛠️ Debugging & Testing

### **Read Serial Debug Output**

```sh
pio device monitor -b 115200 -p /dev/ttyACM0
```

### **Manually Send ANT+ Test Message**

```sh
stty -F /dev/ttyACM0 115200 raw 
printf "\xA4\x09\x1E\x00\x00\x45\x23\x55\x67\x89\xCA" > /dev/ttyACM0
```

✅ Should display **Parsed ANT+ Data: Speed=XX km/h, Cadence=XX RPM**  

### **Reboot ESP32-S3 via Serial**

```sh
printf "\xA4\x01\xF1\xXX" > /dev/ttyACM0
```

✅ ESP32-S3 will reboot and log **"[DEBUG] Reboot Command Received"**  

## 🔜 Roadmap & Future Enhancements

🔹 **Multi-Device Support** – Pair multiple ANT+ sensors at the same time  
🔹 **Power Optimization** – Implement ESP32-S3 deep sleep mode  
🔹 **USB Host Mode** – Directly connect an ANT+ USB dongle to ESP32-S3  
🔹 **More ANT+ Profiles** – Add HRM & Power Meter support  

## 📝 License

This project is **open-source** under the MIT License.

---

🎯 **Now your ESP32-S3 is a fully functional ANT+ to BLE FTMS bridge!** 🚴‍♂️🔥  

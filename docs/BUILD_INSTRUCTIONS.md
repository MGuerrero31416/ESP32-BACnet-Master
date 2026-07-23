# ESP32 BACnet Device - Build Instructions

## Prerequisites
- ESP-IDF v5.5.1 installed
- WiFi credentials configured in main.c

## Build Steps

1. **Configure WiFi credentials** in `main/main.c`:
   ```c
   #define WIFI_SSID "Your_WiFi_Name"
   #define WIFI_PASS "Your_Password"
   ```

2. **Open ESP-IDF PowerShell** (or source the export script)

3. **Navigate to project directory**:
   ```powershell
   cd C:\esp\BACnet-ESP32-S3
   ```

4. **Build the project**:
   ```powershell
   idf.py build
   ```

5. **Flash to ESP32** (replace COM3 with your port):
   ```powershell
   idf.py -p COM3 flash
   ```

6. **Monitor output**:
   ```powershell
   idf.py -p COM3 monitor
   ```

## Expected Output

The device will:
- Connect to WiFi
- Initialize 6 Analog Value objects
- Initialize 6 Binary Value objects
- Send I-Am broadcasts every 30 seconds on UDP port 47808
- Display object status every 10 seconds

## BACnet Configuration

- **Device Instance**: 1234
- **Device Name**: ESP32-BACnet
- **Port**: 47808 (0xBAC0)
- **Objects**: 
  - 6 Analog Values (instances 0-5)
  - 6 Binary Values (instances 0-5)

## Testing

Use BACnet tools like:
- YABE (Yet Another BACnet Explorer)
- BACnet Scanner
- Visual Test Shell

The device should appear as "ESP32-BACnet" with device instance 1234.

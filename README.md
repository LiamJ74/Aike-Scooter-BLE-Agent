# Äike Scooter Agent for Flipper Zero

This application allows you to scan for and control Äike e-scooters using Bluetooth Low Energy (BLE).

## ⚠️ IMPORTANT: Custom Firmware Required

This application requires a Flipper Zero custom firmware that exposes the full BLE Central/GATT Client API (GAP/GATT headers).
It has been designed for and tested on:
- **Momentum Firmware** (Recommended)
- **RogueMaster Firmware**
- **Xtreme Firmware** (or Unleashed)

**It will NOT compile or run on the official Flipper Zero firmware** because the official SDK does not expose the necessary BLE `gap.h` and `ble_app.h` interfaces for third-party applications to act as a BLE Central device.

## Features

- **Scan**: Auto-detects scooters named "AIKE*".
- **Control**:
  - Unlock / Lock
  - Eco Mode (ON/OFF)
  - Open Battery Compartment
- **Security**: Implements the required SHA1 Challenge-Response handshake using the known Master Key.

## Installation & Compilation

### Prerequisites
- A Flipper Zero running **Momentum Firmware** (or compatible custom FW).
- `ufbt` (uFlipper Build Tool) installed on your computer.
- Python 3.

### Compilation Steps

1. **Install ufbt**:
   ```bash
   pip3 install ufbt
   ```

2. **Clone this repository**:
   ```bash
   git clone <repo_url>
   cd <repo_directory>
   ```

3. **Update SDK for Custom Firmware**:
   Since this app depends on custom firmware headers, you must point `ufbt` to the Momentum SDK (or your specific firmware's SDK).
   ```bash
   # For Momentum Firmware
   ufbt update --index-url=https://up.momentum-fw.dev/firmware/directory.json
   ```
   *If you are unsure, check the developer documentation of your specific firmware for the correct `ufbt update` command.*

4. **Build and Launch**:
   Connect your Flipper Zero via USB and run:
   ```bash
   ufbt launch
   ```
   This will compile the app, upload it to the Flipper, and start it immediately.

### Manual Installation (.fap)

If you have a pre-compiled `.fap` file:
1. Connect Flipper via USB or insert SD card into PC.
2. Copy `aike_agent.fap` to `SD Card/apps/Bluetooth/`.
3. Open the file browser on Flipper, navigate to the file, and run it.

## Disclaimer

This tool is for educational purposes and for controlling your own vehicle. Using this tool on scooters you do not own may be illegal in your jurisdiction.

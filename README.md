# Äike Scooter Agent for Flipper Zero

This application allows you to scan for and control Äike e-scooters using Bluetooth Low Energy (BLE).

## ⚠️ IMPORTANT: Platform Limitation

**Functionality Warning:**
The Scanning and Connection features of this application rely on **BLE Central / GATT Client** roles.
Currently, the standard Flipper Zero firmware (including official and most custom builds like Momentum/Unleashed) **does not expose** the necessary APIs (`ble_client`, `scan`, `connect`) to user-space applications.

As a result:
- The application will **compile and launch** safely.
- **Scanning will fail gracefully** (stubbed), logging a warning that the API is disabled.
- To make this fully functional, you must build the firmware yourself enabling `hci_send_req` or wait for an SDK update that exposes a `ble_central` API.

## Features

- **Scan**: Auto-detects scooters named "AIKE*" (Stubbed - requires custom FW with HCI enabled).
- **Control**:
  - Unlock / Lock
  - Eco Mode (ON/OFF)
  - Open Battery Compartment
- **Security**: Implements the required SHA1 Challenge-Response handshake using the known Master Key.

## Installation & Compilation

### Prerequisites
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

3. **Update SDK**:
   ```bash
   # For Momentum Firmware
   ufbt update --index-url=https://up.momentum-fw.dev/firmware/directory.json
   ```

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

# Äike Scooter Agent for Flipper Zero

This application allows you to scan for and control Äike e-scooters using Bluetooth Low Energy (BLE).

**⚠️ IMPORTANT: Custom Firmware Required**

This application requires a Flipper Zero custom firmware that exposes the full BLE Central/GATT Client API (GAP/GATT headers).
It has been designed for and tested on:
- **Momentum Firmware**
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

## Installation / Compilation

1. Ensure you have `ufbt` installed.
2. Clone this repository.
3. Switch your `ufbt` target to your custom firmware SDK (e.g., `ufbt update --channel=dev` if pointing to a compatible repo, or manually link against your firmware source).
4. Run `ufbt launch`.

## Disclaimer

This tool is for educational purposes and for controlling your own vehicle. Using this tool on scooters you do not own may be illegal in your jurisdiction.

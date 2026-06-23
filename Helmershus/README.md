# Helmershus

Hardware test scripts for integrating two RFID systems. Currently focused on Nexmosphere (XN-115 + XR RFID).

## Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Nexmosphere test

List serial ports:

```bash
ls /dev/tty.*
```

Run listener (default port is set in `nexmosphere_test.py`):

```bash
python nexmosphere_test.py
```

Override port:

```bash
python nexmosphere_test.py /dev/tty.PL2303G-USBtoUART14410
```

Serial: 115200 8N1, CR+LF. Place/remove RFID tags to see events (e.g. `X001B[TD=UID:...]`, `XR[PU002]`). Ctrl+C to stop.

## Hardware

- XN-115 controller (USB via Prolific PL2303)
- XR-2 RFID driver + antenna on X-talk interface 001

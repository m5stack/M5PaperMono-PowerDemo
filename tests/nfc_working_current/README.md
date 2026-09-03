# NFC Working Current

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current with the ST25R3916 NFC reader in either power-down or NFC-A polling mode.

## Enabled Modules

- `M5UnitUnified` and `M5UnitUnifiedNFC`: ST25R3916 reader control
- `m5pm_phase_test`: shared awake L2 baseline
- `m5pm_power`: NFC rail restore through IOE1 and PM1
- `m5pm_test_support`: configuration ID, test markers and safe hold state

## Module Configuration

| Module | Configuration |
|---|---|
| ST25R3916 | I2C address `0x50`, NFC-A mode, emulation disabled, IRQ use disabled; supplied by `3V3_L2` through IOE1 `PYB_NFC_EN` and controlled over the shared I2C bus |
| NFC power-down | Field disabled, `CMD_STOP_ALL_ACTIVITIES` sent, operation-control register written and read back as `0x00` |
| NFC-A polling | 100 ms detection timeout followed by a 20 ms gap; a card must be absent before and during capture |
| I2C | Shared board bus at 100 kHz; NFC reader is attached through `M5UnitUnified` |
| Firmware log | Power-down: `BOARD_OK NFC=power_down operation_control=0x%02X`; polling: `BOARD_OK NFC=nfc_a_polling poll_period_ms=120 card=absent` |

## Runtime Mode

The selected mode is passed to test support as `config_id`. The test prepares the awake L2 baseline, restores the NFC rail, waits 120 ms for settling, initializes the reader, and emits `TEST_READY`. Power-down then holds the reader idle. Polling repeatedly calls NFC-A detection with a 100 ms timeout and 20 ms gap; a detected card causes the test to enter its safe hold state.

## Operating Modes

Select one Kconfig defaults file before building:

| `config_id` | Kconfig selection | Operating state |
|---|---|---|
| `nfc-power-down` | `CONFIG_M5PM_NFC_MODE_POWER_DOWN=y` | Field disabled, all activities stopped, operation-control `0x00` |
| `nfc-a-polling` | `CONFIG_M5PM_NFC_MODE_NFC_A_POLLING=y` | NFC-A polling, no card present |

Example:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"
idf.py reconfigure
```

In Bash, use `rm -f sdkconfig`, then `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"` and run `idf.py reconfigure`.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

| Mode | 3.7 V | 4.2 V |
|---|---:|---:|
| `nfc-power-down` | 34.35 mA | 32.39 mA |
| `nfc-a-polling` | 109.31 mA | 103.21 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `nfc_working_current` |
| Config ID | `nfc-power-down` or `nfc-a-polling` |
| Reader address | ST25R3916 address is selected by the M5UnitUnifiedNFC component |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then select the desired defaults file and build here:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, the mode-specific `BOARD_OK`, and `TEST_READY` over USB Type-C. Disconnect USB Type-C before formal current measurement and keep the NFC antenna area free of cards for polling captures.

## Expected Validation

Power-down requires the operation-control readback `0x00`. Polling requires `card=absent` and a repeating 100 ms detect plus 20 ms gap loop.

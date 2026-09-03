# LoRa RX Idle

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware keeps the SX1262 in continuous receive without test packets and measures whole-board input current in the receive-idle state.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: awake L2 baseline and 90-second measurement timing
- `m5pm_lora` and `m5pm_power`: SX1262 receive mode and LoRa rail control

## Module Configuration

| Module | Configuration |
|---|---|
| SX1262 | 868 MHz, 125 kHz, SF12, CR 4/5, sync word `0x34`, TX power 22 dBm; SPI3 SCLK GPIO39, MOSI GPIO38, MISO GPIO40, NSS GPIO41, IRQ GPIO5, BUSY GPIO21; rail `3V3_L2_LoRa` enabled through PM1 GPIO2 |
| RF support | Preamble 8 symbols, TCXO 3.0 V, LDO regulator |
| Board control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared 100 kHz I2C bus; IOE1 controls LoRa reset and antenna-switch pins |
| Radio state | Continuous receive; TX, ACK and retries disabled; test packets absent |
| Measurement | 90,000 ms wait before stopping the radio and powering off the LoRa rail |
| Firmware log | `BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo STATE=continuous_rx TEST_PACKETS=0 TX=0 ACK=0 RETRY=0` |

## Runtime Mode

The test restores the LoRa rail, initializes SX1262, enters continuous receive, emits `BOARD_OK` and `TEST_READY`, waits 90 s, then stops the radio and powers off the LoRa rail.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 42.02 mA
- 4.2 V: 39.89 mA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `lora_rx_idle` |
| Config ID | `lora_rx_idle` |

## Build and Flash

Use ESP-IDF 5.5.5 from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Disconnect USB Type-C before formal power measurement and do not run the serial monitor during measurement.

## Expected Validation

Confirm `TEST_START`, the test-specific `BOARD_OK` fields, and `TEST_READY`. The firmware emits a failure marker if initialization or validation cannot proceed.

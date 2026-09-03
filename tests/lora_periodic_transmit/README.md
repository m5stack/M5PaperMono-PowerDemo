# LoRa Periodic Transmit

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures both the steady periodic-transmit workload and one complete SX1262 transmission event.

## Enabled Modules

- `m5pm_lora`: SX1262 initialization, transmit, stop and power-control interface
- `m5pm_phase_test`: awake L2 baseline, stop-button handling and safe hold state
- `m5pm_power`: LoRa rail restore and power-off through PM1/IOE1
- `m5pm_test_support`: test markers and validation context

## Module Configuration

| Module | Configuration |
|---|---|
| SX1262 | 868 MHz, 125 kHz bandwidth, SF12, CR 4/5, sync word `0x34`, TX power 22 dBm; SPI3 SCLK GPIO39, MOSI GPIO38, MISO GPIO40, NSS GPIO41, IRQ GPIO5, BUSY GPIO21; rail `3V3_L2_LoRa` enabled through PM1 GPIO2 |
| RF support | Preamble 8 symbols, TCXO 3.0 V, LDO regulator |
| Board control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared 100 kHz I2C bus; IOE1 controls LoRa reset and antenna-switch pins |
| Payload | ASCII `PM_n`, sequence number starts at 0 |
| Schedule | 1,000 ms wait after each completed TX; 10,000 ms per-TX timeout; 90,000 ms runtime limit |
| Stop input | Key1 or Key2, configured by `configure_stop_buttons()` |
| Firmware log | `BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo PAYLOAD=PM_n PERIOD_AFTER_TX_MS=1000 ACK=0 RETRY=0 LOCAL_TX_DONE=required RUNTIME_LIMIT_MS=90000 STOP=key1_or_key2` |

## Runtime Mode

The test restores the LoRa rail, configures the stop buttons, initializes the radio, and emits `TEST_READY`. It transmits sequential `PM_n` payloads and waits 1 s between completed transmissions until the 90 s limit or a stop-button request. The radio is stopped and powered off before the firmware enters its safe hold state.

## Measured Current

The periodic result is the `WINDOW Avg` from the approximately 60-second capture. The single-transmission result uses a `CURSOR Avg` covering one TX event.

| Measurement | 3.7 V | 4.2 V |
|---|---:|---:|
| Periodic transmit (`WINDOW Avg`) | 61.25 mA | 59.50 mA |
| Single transmission (`CURSOR Avg`) | 96.55 mA for 0.840053 s; 22.52891 uAh | 93.52 mA for 0.840053 s; 21.82189 uAh |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `lora_periodic_transmit` |
| Config ID | `lora_periodic_transmit` |
| Radio state after test | SX1262 stopped; LoRa rail powered off |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then build from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, the complete `BOARD_OK` radio parameters, and `TEST_READY` over USB Type-C. Disconnect USB Type-C before measurement. For the single-event capture, select the cursor around exactly one TX pulse as shown in the graph index.

## Expected Validation

Each transmitted payload must complete locally before the 1 s wait begins. A manual stop is represented by the configured Key1/Key2 input; otherwise the runtime limit ends the periodic sequence.

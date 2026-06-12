# AMCISS DPM

> **Scope:** This document covers the DPM firmware only and is intended for developers continuing work on this codebase. For hardware setup, physical installation, IP configuration, flashing instructions, and GUI usage refer to the AMCISS Setup and User Guide.

---

## System Overview

The **Data Processing Module (DPM)** is one of three networked subsystems in AMCISS (Adaptive Multichannel Inductive Sorting System). It runs on an STM32H723ZGT6 (NUCLEO-H723ZG) under Azure RTOS (ThreadX + NetX Duo).

Its responsibilities are:

1. Receive `AMCISS_Packet_t` UDP packets from two upstream **Data Collection Modules** (DCM1 and DCM2), each reporting 32 inductive sensors.
2. Merge the two 32-sensor streams into a single 64-sensor picture.
3. Apply per-coil time delays to compensate for the physical stagger of each sensor coil along the conveyor.
4. Detect metal presence across the delay-aligned sensor array and drive a flap actuator (via GPIO → PLC) with configurable timing.
5. Forward a merged `DPM_Forward_Packet_t` to a PC for GUI visualisation.

**Important:** Any code regeneration via STM32CubeMX or STM32CubeIDE will result in code in app_netxduo.c and app_netxduo.h to be reverted to the original state. If this happens, simply recopy and paste the code from this repository to restore functionality. 
---

## Project File Tree

```
DPMProj/
├── Application/
│   └── User/
│       ├── AZURE_RTOS/
│       └── Core/
│           ├── app_threadx.c         ThreadX initialisation
│           ├── detection.c           Metal detection logic
│           ├── detection.h
│           ├── main.c                Entry point, peripheral init (GPIO, ETH, USART3)
│           ├── stm32h7xx_hal_msp.c   HAL MSP peripheral init callbacks
│           ├── stm32h7xx_hal_timebase_tim.c
│           ├── stm32h7xx_it.c        Interrupt handlers
│           ├── syscalls.c
│           ├── sysmem.c
│           └── tx_initialize_low_level.S
├── NetXDuo/
│   └── App/
│       └── app_netxduo.c             Network stack, packet processing, flap control
├── Drivers/                          STM32 HAL + LDC1101 driver
│   └── LDC1101/
│       ├── __ldc1101_driver.c
│       ├── __ldc1101_driver.h
│       ├── __ldc1101_hal.c
│       └── __ldc1101_hal.h
├── Middlewares/                      Azure RTOS (ThreadX / NetX Duo)
├── Doc/                              User guide and supporting documentation
├── Debug/
├── DPMProj Debug.launch
├── Nx_TCP_Echo_Client Debug.launch
└── STM32H723ZGTX_FLASH.ld
```
Many more files are included in this repository, however none were modified by the team. The main files in use are main.c and app_netxduo.c/h. All code for DPM operation is present there.
---

## Key Source Files

### `main.c`
Entry point. Initialises all peripheral functions.

> **Note:** `printf` routes to USART3 via `HAL_UART_Transmit`. At the ~1000 packets/second operating rate, any `printf` inside the packet processing loop will stall the system. Keep USART output outside the hot path.

---

### `app_netxduo.c` + `app_netxduo.h`
The main application. Three RTOS threads are created:

| Thread | Priority | Role |
|---|---|---|
| `NxAppThread` | 10 | Sets static IP, waits for link, resumes UDP thread |
| `AppUDPThread` | 6 | Packet receive → merge → delay align → flap → forward |
| `AppLinkThread` | 11 | Monitors Ethernet cable state, reapplies static IP on reconnect |

#### Network Topology

```
DCM1 (192.168.0.50)  ──┐
                        ├──► DPM (192.168.0.51) :5005 ──► PC (192.168.0.69) :5005
DCM2 (192.168.0.52)  ──┘
```

Static IP addresses are used for all subsystems and they must be aligned as such. The user does not have to modify any IP address information

#### Ethernet packet Structs (`app_netxduo.h`)


```c
// Received from each DCM (968 bytes)
AMCISS_Packet_t {
    magic[2], seq, timestamp_ms,
    ldc[32], rp[32],
    freq_hz[32], deviation[32], threshold[32],
    baseline[32], raw_mean[32], filt_std[32],
    is_metal[32], calib_done[32]
}

// Forwarded to PC (332 bytes)
DPM_Forward_Packet_t {
    magic[2], seq, timestamp_ms,
    ldc[64], rp[64],
    is_metal[64]
}
```

#### Coil Delay Compensation

The 32 sensor coils are physically staggered along the conveyor. To determine whether metal is present at the reference position (coils 15/16, offset = 0.0 mm), each coil's reading is retrieved from a different historical position in a ring buffer:

```
delay_packets[i] = (int)(COIL_N_OFFSET / (VELOCITY * PACKET_RATE_MS))
```

All offsets and the velocity/rate constants are defined in `app_netxduo.h`. `DELAY_COUNT` (220) is the ring buffer depth and must be ≥ the largest delay value — currently ~217 packets for coils 17/18 at 390.857 mm offset. This must be modified depending on flap speed and is currently calibrated for the top conveyor belt speed available.

The ring buffer (`LDC_array[64][DELAY_COUNT]`) stores the `is_metal` boolean for each sensor per packet. Processing is held back for the first `DELAY_COUNT` packets to allow the buffer to prime before delay lookups are valid.

#### Flap Control

```
Idle
 ├─ any coil detects metal (delay-aligned) → rising edge latched
 │     count down FLAP_TRIGGER_DELAY_MS (450 ms ≈ 500 packets)
 │     → FLAP_PIN HIGH (flap opens)
 │
 └─ metal signal clears → falling edge latched
       count down (FLAP_TRIGGER_DELAY_MS + 450) packets
       → FLAP_PIN LOW (flap closes)
```

`rising_edge_detected` and `falling_edge_detected` are latch flags — a new edge is ignored until the current one resolves.

#### Configuration Constants

| Constant | Value | Meaning |
|---|---|---|
| `VELOCITY` | 2.0 | Conveyor speed (mm/ms) |
| `PACKET_RATE_MS` | 0.9f | Expected packet interval (ms) |
| `FLAP_TRIGGER_DELAY_MS` | 450 | Delay from first detection to flap open (ms) |
| `DELAY_COUNT` | 220 | Ring buffer depth — must cover max coil delay |
| `DEFAULT_PORT` / `PC_PORT` | 5005 | UDP receive and forward port |
| `STM_IP_ADDRESS` | 192.168.0.51 | DPM static IP |
| `PC_IP_ADDRESS` | 192.168.0.69 | Forwarding target |
| `COIL_N_OFFSET` | (varies) | Physical distance of coil N from reference (mm) |

---

### `detection.c` / `detection.h`
Metal detection logic, separated from the network code. See inline comments for thresholding and calibration details.

---

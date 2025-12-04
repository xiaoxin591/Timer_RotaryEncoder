# Copilot Instructions - Timer RotaryEncoder Project

## Project Overview
This is an **STM32F103C8 embedded firmware project** for controlling a servo motor with rotary encoder input, push buttons, Bluetooth communication, and OLED display output. The system operates in two modes: **Setting Mode** (adjust parameters via encoder) and **Running Mode** (execute servo motion with timing).

## Architecture & Key Components

### Core State Machine (Servo Control)
The system has three servo states managed in `Core/Src/main.c`:
- **SERVO_IDLE**: Servo stopped, holding current position
- **SERVO_RETURNING**: Servo returning to 0° (homing phase before motion cycle)
- **SERVO_RUNNING**: Servo executing reciprocal motion within `servo_angle_range`

Critical functions:
- `UpdateServoPosition()`: Main state machine update (called in main loop ~100ms cycle)
- `SetServoAngle(angle)`: Sets PWM via TIM4 Channel 3 to position servo
- `MapAngleToPWM(angle)`: Converts 0-180° angles to PWM pulse width (1000-2000 microseconds)

### Input Interfaces
1. **Rotary Encoder (TIM1 - encoder mode)**
   - Direct angle mapping: encoder count (0-180) = servo angle in degrees
   - In Setting Mode, encoder directly sets `servo_angle_range`
   - Read via `__HAL_TIM_GET_COUNTER(&htim1)` in main loop

2. **Push Buttons (GPIO - PB12, PB13)**
   - Button1: Start/exit setting mode
   - Button2: Long press (2s) enters Setting Mode; short press stops servo
   - Uses debouncing (200ms threshold in polling loop, separate from EXTI handlers)

3. **Bluetooth UART (UART3 - DMA receive to idle)**
   - Receives commands: `A<angle>` (A90), `T<minutes>`, `S<seconds>`, START, STOP
   - Parsed by `ParseBluetoothData()` which validates ranges and updates servo parameters
   - Sends ACK/ERROR responses via UART2 (debug output)

### Output Interfaces
1. **PWM Servo Control (TIM4 CH3 - PA9)**
   - Prescaler: 720-1, Period: 2000-1 (50Hz frequency)
   - PWM range: 1000-2000 counts (1-2ms pulse width)
   - Constants: `SERVO_PWM_MIN=1000`, `SERVO_PWM_MAX=2000`, `SERVO_MAX_ANGLE=180`, `SERVO_MIN_ANGLE=0`

2. **Debug UART (UART2)**
   - Sends state messages, encoder values, servo angles via `HAL_UART_Transmit()` with `strlen()` for length
   - All formatted strings use `snprintf()` with buffer size checks (no hardcoded lengths after recent security patch)

3. **OLED Display (I2C - 128x64 SSD1306)**
   - **Setting Mode**: Shows "SETTING MODE", current angle (0-180), time (minutes), speed
   - **Running Mode**: Shows "RUNNING MODE", servo state (IDLE/RETURNING/RUNNING), current angle, target angle range
   - Updated via `OLED_NewFrame()` → `OLED_PrintString()` → `OLED_ShowFrame()` in main loop

## Critical Data Flow

### Servo Motion Cycle
```
1. User sets servo_angle_range (0-180°) via encoder or Bluetooth
2. Button1 triggers SERVO_RETURNING state
3. Servo decrements from current angle to 0° (one per cycle)
4. At 0°, transitions to SERVO_RUNNING
5. Executes reciprocal motion: 0° → servo_angle_range → 0° using progress ratio
6. Cycle duration = servo_motion_time * 2 (e.g., 5 seconds per direction = 10s total)
7. Total runtime = servo_run_duration * 60 * 1000ms (checked via HAL_GetTick)
8. Stops at servo_run_end_time, returns to SERVO_IDLE
```

### Global State Variables (key in main.c)
- `servo_state`: Current servo FSM state
- `servo_angle_range`: 0-180°, set by encoder or Bluetooth command
- `servo_current_angle`: Current position (0-180°)
- `servo_motion_time`: Motion time per direction in milliseconds (Bluetooth: S<1-10>)
- `servo_run_duration`: Total runtime in minutes (Bluetooth: T<1-60>)
- `servo_start_time`, `servo_run_end_time`: Timing for session duration
- `setting_mode`: Boolean flag (0=Run mode, 1=Setting mode)
- `bluetooth_angle`, `bluetooth_time`, `bluetooth_speed`: Parsed values from Bluetooth
- `receiveDate[256]`: Bluetooth receive buffer for DMA

## Development Conventions

### Safety & Security Patterns
- **Always use `snprintf(buffer, sizeof(buffer), format, ...)` for string formatting** (prevents buffer overflow)
- **Use `strlen()` for HAL_UART_Transmit length** instead of hardcoded integers
- Angle clamping: `if(angle > 180) angle = 180; if(angle < 0) angle = 0;`

### Timing & Polling
- Main loop cycle: ~100ms (controlled by `HAL_Delay(100)` or update rates)
- Button debouncing: 50ms for EXTI, 200ms for polling mode
- Bluetooth polling: 10ms check interval via `if(HAL_GetTick() - last_bt_check > 10)`
- Encoder polling: Checks for count change each cycle via `__HAL_TIM_GET_COUNTER()`

### UART Communication Protocol
- All strings end with `\r\n` for terminal/logging compatibility
- Bluetooth command format: Single letter prefix + optional value (e.g., "A90", "T15", "S5")
- Responses are ACK/ERROR messages sent back via UART2 debug channel

### Common Modifications
1. **Change angle range** → Modify `SERVO_MAX_ANGLE` constant (default 180)
2. **Adjust motion smoothness** → Modify motion calculation in `UpdateServoPosition()` (currently uses phase progress ratio)
3. **Add new Bluetooth commands** → Edit `ParseBluetoothData()` with new prefix checks
4. **Change timing parameters** → Edit `servo_run_duration`, `servo_motion_time` globals or constants

## Files Structure
- `Core/Src/main.c`: Main logic, servo state machine, button handlers, Bluetooth parser
- `Core/Src/tim.c`: TIM1 (encoder), TIM4 (PWM servo) initialization
- `Core/Src/oled.c`: OLED graphics library with frame buffer
- `Core/Src/usart.c`: UART configuration (UART2 debug, UART3 Bluetooth)
- `Core/Src/i2c.c`: I2C for OLED communication
- `Core/Inc/font.h`: ASCII/CJK font definitions for OLED text rendering

## Build & Debug
- **Toolchain**: STM32CubeMX project with arm-none-eabi-gcc
- **Debug Output**: UART2 (USB serial) - connect to see state messages
- **Flash**: STM32F103C8TX (64KB FLASH, 20KB RAM)
- **Makefile-based build** in `Debug/` directory

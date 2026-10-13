# STM32C031C6 FreeRTOS Real-Time Monitoring System

A real-time monitoring and control application developed using **C/C++, FreeRTOS, and the STM32C031C6 microcontroller**. This project demonstrates multitasking, queue-based inter-task communication, mutex synchronization, periodic task scheduling, threshold-based LED control, and runtime health monitoring.

The application is simulated using **Wokwi** with the STM32 Arduino core and STM32duino FreeRTOS library.

## Overview

The system uses four independent FreeRTOS tasks to simulate sensor data acquisition, process measurements, monitor system activity, and report information through USART2.

Temperature and humidity values are generated in software. The Control Task receives these values through a FreeRTOS queue, evaluates the temperature against a predefined threshold, and controls the LED accordingly.

## Features

- Four independent FreeRTOS tasks with configurable priorities.
- Queue-based communication between sensor and control tasks.
- Five-entry queue for buffering sensor data.
- Mutex-protected UART output to prevent interleaved task messages.
- Periodic sensor-data generation using `vTaskDelayUntil()`.
- Temperature-threshold monitoring and LED control.
- USART2 serial monitoring at 115200 baud.
- Runtime counters for queue transfers and communication errors.
- Periodic system health reports.
- Simulation and testing using Wokwi.

## System Block Diagram

The system-level data flow is:

```text
       STM32C031C6 Microcontroller
                   |
             FreeRTOS Kernel
                   |
       +-----------+-----------+
       |           |           |
   Sensor Task  UART Task  Health Task
       |
       | SensorData
       v
  FreeRTOS Queue
   (5 entries)
       |
       v
   Control Task
       |
       +------------------+
       |                  |
 Temperature Check    UART Report
       |
       v
    LED Output

UART Task, Control Task, and Health Task
              |
          UART Mutex
              |
              v
       USART2 Serial Monitor
```

## Software Architecture

The application consists of four FreeRTOS tasks.

| Task | Priority | Function |
|---|---:|---|
| Sensor Task | 2 | Generates temperature and humidity values and sends samples to the queue every 2 seconds. |
| Control Task | 2 | Receives samples, checks the temperature threshold, controls the LED, and prints reports. |
| UART Task | 1 | Prints a periodic UART status message every second. |
| Health Task | 1 | Reports sample counters and queue error statistics every 5 seconds. |

### 1. Sensor Task

The Sensor Task simulates sensor readings using a `SensorData` structure containing temperature and humidity.

```cpp
typedef struct {
    float temperature;
    float humidity;
} SensorData;
```

The task generates updated values every 2 seconds and sends them to the queue using `xQueueSend()`.

A nonblocking send allows the application to detect queue-full conditions and increment the corresponding error counter.

### 2. FreeRTOS Queue

The queue transfers sensor data from the Sensor Task to the Control Task.

- Capacity: 5 entries.
- Data type: `SensorData`.
- Producer: Sensor Task.
- Consumer: Control Task.

The Control Task uses `xQueueReceive()` to wait for new data rather than continuously polling for available samples.

### 3. Control Task

The Control Task processes each received sample and compares the temperature against a threshold of 30°C.

| Condition | System status | LED |
|---|---|---|
| Temperature below 30°C | NORMAL | OFF |
| Temperature at or above 30°C | HIGH TEMPERATURE | ON |

The task reports temperature, humidity, sample information, and the current system status through USART2.

### 4. UART Task

The UART Task periodically prints a status message to demonstrate independent task execution and serial logging under FreeRTOS.

### 5. Health Task

The Health Task periodically reports:

- Successfully transmitted sensor samples.
- Samples received by the Control Task.
- Queue-send errors.
- Queue-receive errors.
- Basic system health status.

A `SYSTEM HEALTH: OK` message indicates that the monitored queue-error counters are zero. It does not represent a comprehensive task watchdog or deadline monitor.

## Inter-Task Synchronization

### Queue

A FreeRTOS queue transfers structured sensor data between tasks and allows temporary buffering when the producer and consumer execute at different times.

### Mutex

A FreeRTOS mutex protects shared UART output. Tasks acquire the mutex before printing and release it afterward, reducing the risk of interleaved log messages.

The queue handles **data transfer**, while the mutex handles **shared-resource protection**.

## Timing and Scheduling

| Operation | Period or behavior |
|---|---|
| Sensor-data generation | 2 seconds |
| UART status message | 1 second |
| Health report | 5 seconds |
| Control processing | When a queue item becomes available |
| LED control | Based on the received temperature |

The Sensor Task uses `vTaskDelayUntil()` for periodic scheduling. Actual execution timing depends on the FreeRTOS configuration, task priorities, and system load.

## Technology Stack

- **Microcontroller:** STM32C031C6
- **Development board target:** NUCLEO-C031C6
- **Programming language:** C/C++
- **Real-time operating system:** FreeRTOS
- **Development and simulation:** Wokwi
- **Framework:** STM32 Arduino core
- **Serial communication:** USART2
- **Concurrency mechanisms:** Tasks, queues, and mutexes
- **Output interface:** Serial monitor and LED

## Project Structure

```text
STM32C031C6-FreeRTOS-Monitor/
├── sketch.ino
├── libraries.txt
├── README.md
├── docs/
│   ├── block-diagram.png
│   └── architecture.png
└── images/
    ├── wokwi-simulation.png
    ├── serial-monitor.png
    └── led-threshold.png
```

The diagram and screenshot files are optional until you upload them. Update the filenames in this README if your actual repository structure differs.

## How to Run the Project

1. Open the project in Wokwi with the STM32C031C6 target.
2. Ensure `libraries.txt` includes the STM32duino FreeRTOS library.
3. Build and start the simulation.
4. Open the serial monitor and set the baud rate to 115200.
5. Observe the generated sensor readings, control reports, UART messages, and health statistics.
6. Verify that the LED turns ON when the simulated temperature reaches 30°C.

## Testing and Observations

The simulation was used to verify:

- Creation and execution of the four FreeRTOS tasks.
- Sensor-data transfer through the queue.
- Temperature-threshold detection.
- LED state changes.
- Serial output from multiple tasks.
- Periodic health reports and queue-error counters.

In the observed simulation, sent and received sample counts matched, the queue-error counters remained at zero, and the LED activated at the configured temperature threshold.

## Current Limitations

- Temperature and humidity values are simulated in software rather than measured by physical sensors.
- Health monitoring covers selected queue counters and does not provide comprehensive task-runtime or deadline supervision.
- Physical hardware behavior has not been validated as part of this simulation.

## Future Improvements

- Integrate a physical temperature and humidity sensor.
- Add sensor validation and fault-handling logic.
- Improve synchronization for shared diagnostic counters.
- Implement task watchdogs and execution-time monitoring.
- Add configurable thresholds and additional fault states.
- Measure task stack usage and evaluate resource utilization.
- Extend testing to cover queue-full conditions and sensor failures.

## Learning Outcomes

This project provides practical experience with:

- FreeRTOS task creation, priorities, and scheduling.
- Queue-based inter-task communication.
- Mutexes and shared-resource synchronization.
- Periodic execution and tick-based timing.
- GPIO-based control logic.
- USART2 serial debugging.
- Runtime diagnostics and basic error monitoring.

## Author

**Moimon Mandal**

GitHub: [@moimon718](https://github.com/moimon718)

---

*This project was developed for learning and demonstrating embedded real-time operating system concepts on an STM32 microcontroller.*

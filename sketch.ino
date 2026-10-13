
#include <Arduino.h>
#include <STM32FreeRTOS.h>

// ============================================================
// PROJECT: STM32C031C6 FREERTOS REAL-TIME MONITORING SYSTEM
// Board: NUCLEO-C031C6
// UART: USART2
// ============================================================

// SENSOR DATA
typedef struct
{
    float temperature;
    float humidity;
} SensorData;

// FREERTOS OBJECTS
QueueHandle_t sensorQueue;
SemaphoreHandle_t uartMutex;

TaskHandle_t uartTaskHandle;
TaskHandle_t sensorTaskHandle;
TaskHandle_t controlTaskHandle;
TaskHandle_t healthTaskHandle;

// HEALTH COUNTERS
volatile uint32_t sensorSendErrors = 0;
volatile uint32_t controlReceiveErrors = 0;
volatile uint32_t sensorSamplesSent = 0;
volatile uint32_t controlSamplesReceived = 0;

// ============================================================
// THREAD-SAFE UART LOGGER
// ============================================================

void UartLog(const char *message)
{
    if (xSemaphoreTake(uartMutex, portMAX_DELAY) == pdTRUE)
    {
        Serial2.println(message);
        xSemaphoreGive(uartMutex);
    }
}

// ============================================================
// TASK 1: UART MONITOR
// ============================================================

void UartTask(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        UartLog("UART Task Running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================
// TASK 2: SENSOR
// ============================================================

void SensorTask(void *pvParameters)
{
    (void)pvParameters;

    SensorData data = {25.0f, 60.0f};

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplePeriod = pdMS_TO_TICKS(2000);

    while (1)
    {
        data.temperature += 1.0f;
        data.humidity += 0.5f;

        // Non-blocking send lets us detect a full queue.
        if (xQueueSend(sensorQueue, &data, 0) == pdPASS)
        {
            sensorSamplesSent++;
            UartLog("Sensor data sent to queue");
        }
        else
        {
            sensorSendErrors++;
            UartLog("ERROR: Sensor queue full");
        }

        vTaskDelayUntil(&lastWakeTime, samplePeriod);
    }
}

// ============================================================
// TASK 3: CONTROL
// ============================================================

void ControlTask(void *pvParameters)
{
    (void)pvParameters;

    SensorData receivedData;

    while (1)
    {
        // Wait for sensor data indefinitely.
        if (xQueueReceive(
                sensorQueue,
                &receivedData,
                portMAX_DELAY) == pdPASS)
        {
            controlSamplesReceived++;

            bool highTemperature =
                receivedData.temperature >= 30.0f;

            if (xSemaphoreTake(uartMutex, portMAX_DELAY) == pdTRUE)
            {
                Serial2.println();
                Serial2.println("===== CONTROL TASK =====");

                Serial2.print("Temperature: ");
                Serial2.print(receivedData.temperature, 2);
                Serial2.println(" C");

                Serial2.print("Humidity: ");
                Serial2.print(receivedData.humidity, 2);
                Serial2.println(" %");

                Serial2.print("Samples received: ");
                Serial2.println((uint32_t)controlSamplesReceived);

                if (highTemperature)
                {
                    Serial2.println("STATUS: HIGH TEMPERATURE");
                }
                else
                {
                    Serial2.println("STATUS: NORMAL");
                }

                Serial2.println("========================");

                xSemaphoreGive(uartMutex);
            }

            // Control Task exclusively owns the LED.
            digitalWrite(
                LED_BUILTIN,
                highTemperature ? HIGH : LOW
            );
        }
        else
        {
            // xQueueReceive with portMAX_DELAY should not time out.
            // Count an error if the receive ever fails.
            controlReceiveErrors++;
        }
    }
}

// ============================================================
// TASK 4: SYSTEM HEALTH MONITOR
// ============================================================

void HealthTask(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        if (xSemaphoreTake(uartMutex, portMAX_DELAY) == pdTRUE)
        {
            Serial2.println();
            Serial2.println("===== SYSTEM HEALTH =====");

            Serial2.print("Samples sent: ");
            Serial2.println((uint32_t)sensorSamplesSent);

            Serial2.print("Samples received: ");
            Serial2.println((uint32_t)controlSamplesReceived);

            Serial2.print("Queue send errors: ");
            Serial2.println((uint32_t)sensorSendErrors);

            Serial2.print("Queue receive errors: ");
            Serial2.println((uint32_t)controlReceiveErrors);

            if (sensorSendErrors == 0 &&
                controlReceiveErrors == 0)
            {
                Serial2.println("SYSTEM HEALTH: OK");
            }
            else
            {
                Serial2.println("SYSTEM HEALTH: CHECK ERRORS");
            }

            Serial2.println("========================");

            xSemaphoreGive(uartMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial2.begin(115200);
    delay(500);

    Serial2.println();
    Serial2.println("================================");
    Serial2.println(" STM32C031C6 FreeRTOS Monitor");
    Serial2.println("================================");

    // CREATE QUEUE
    sensorQueue = xQueueCreate(5, sizeof(SensorData));

    if (sensorQueue == NULL)
    {
        Serial2.println("ERROR: Queue creation failed!");
        while (1) {}
    }

    // CREATE UART MUTEX
    uartMutex = xSemaphoreCreateMutex();

    if (uartMutex == NULL)
    {
        Serial2.println("ERROR: Mutex creation failed!");
        while (1) {}
    }

    Serial2.println("Sensor queue created successfully");
    Serial2.println("UART mutex created successfully");

    BaseType_t result;

    // UART TASK: PRIORITY 1
    result = xTaskCreate(
        UartTask, "UART_Task", 256,
        NULL, 1, &uartTaskHandle);
    configASSERT(result == pdPASS);

    // SENSOR TASK: PRIORITY 2
    result = xTaskCreate(
        SensorTask, "Sensor_Task", 256,
        NULL, 2, &sensorTaskHandle);
    configASSERT(result == pdPASS);

    // CONTROL TASK: PRIORITY 2
    result = xTaskCreate(
        ControlTask, "Control_Task", 384,
        NULL, 2, &controlTaskHandle);
    configASSERT(result == pdPASS);

    // HEALTH TASK: PRIORITY 1
    result = xTaskCreate(
        HealthTask, "Health_Task", 384,
        NULL, 1, &healthTaskHandle);
    configASSERT(result == pdPASS);

    Serial2.println("Starting FreeRTOS scheduler...");
    vTaskStartScheduler();

    Serial2.println("ERROR: Scheduler failed!");

    while (1) {}
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // FreeRTOS controls task execution.
}

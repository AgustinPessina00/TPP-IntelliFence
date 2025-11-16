#include "MessageExample.h"
#include <stdio.h>

// ====== Implementación de Tests y Ejemplos ======

void MessageSystem_BasicTest(void) {
    printf("\n=== MessageSystem Basic Test ===\n");
    
    // Test 1: Allocate y Free básico
    EmbeddedMessage_t* msg1 = MessagePool_Allocate();
    if (msg1 == NULL) {
        printf("ERROR: Failed to allocate message 1\n");
        return;
    }
    
    // Crear mensaje básico
    MessageResult_t result = EmbeddedMessage_Create(msg1, MSG_ID_SEND_GPS, 
                                                   MODULE_GPS, MODULE_DISPATCHER);
    if (result != MSG_RESULT_OK) {
        printf("ERROR: Failed to create message. Result: %d\n", result);
        MessagePool_Free(msg1);
        return;
    }
    
    printf("SUCCESS: Created message ID=%d, Sender=%d, Receiver=%d\n", 
           msg1->id, msg1->sender, msg1->receiver);
    
    // Test 2: Mensaje con payload
    uint8_t gps_data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    EmbeddedMessage_t* msg2 = MessagePool_Allocate();
    if (msg2 == NULL) {
        printf("ERROR: Failed to allocate message 2\n");
        MessagePool_Free(msg1);
        return;
    }
    
    result = EmbeddedMessage_CreateWithPayload(msg2, MSG_ID_SEND_GPS,
                                             MODULE_GPS, MODULE_DISPATCHER,
                                             gps_data, sizeof(gps_data));
    if (result != MSG_RESULT_OK) {
        printf("ERROR: Failed to create message with payload. Result: %d\n", result);
        MessagePool_Free(msg1);
        MessagePool_Free(msg2);
        return;
    }
    
    printf("SUCCESS: Created message with payload, length=%d\n", msg2->length);
    printf("Payload: ");
    for (int i = 0; i < msg2->length; i++) {
        printf("0x%02X ", msg2->payload[i]);
    }
    printf("\n");
    
    // Test 3: Copy message
    EmbeddedMessage_t* msg3 = MessagePool_Allocate();
    if (msg3 == NULL) {
        printf("ERROR: Failed to allocate message 3\n");
        MessagePool_Free(msg1);
        MessagePool_Free(msg2);
        return;
    }
    
    result = EmbeddedMessage_Copy(msg3, msg2);
    if (result != MSG_RESULT_OK) {
        printf("ERROR: Failed to copy message. Result: %d\n", result);
    } else {
        printf("SUCCESS: Message copied successfully\n");
    }
    
    // Mostrar estadísticas
    uint32_t total, allocated, max_allocated;
    MessagePool_GetStats(&total, &allocated, &max_allocated);
    printf("Pool Stats - Total: %u, Allocated: %u, Max: %u\n", 
           (unsigned int)total, (unsigned int)allocated, (unsigned int)max_allocated);
    
    // Cleanup
    MessagePool_Free(msg1);
    MessagePool_Free(msg2);
    MessagePool_Free(msg3);
    
    printf("=== Basic Test Completed ===\n\n");
}

void MessageSystem_PerformanceTest(void) {
    printf("\n=== MessageSystem Performance Test ===\n");
    
    const uint32_t NUM_ALLOCATIONS = 100;
    uint32_t start_time = osKernelGetTickCount();
    
    // Test de allocations rápidas
    EmbeddedMessage_t* messages[10]; // Solo testear con 10 mensajes simultáneos
    
    for (int iteration = 0; iteration < NUM_ALLOCATIONS / 10; iteration++) {
        // Allocate batch
        uint32_t alloc_start = osKernelGetTickCount();
        for (int i = 0; i < 10; i++) {
            messages[i] = MessagePool_Allocate();
            if (messages[i] == NULL) {
                printf("WARNING: Allocation failed at iteration %d, message %d\n", iteration, i);
                break;
            }
            
            EmbeddedMessage_Create(messages[i], MSG_ID_SEND_IMU, 
                                 MODULE_SENSOR_ACQ, MODULE_DISPATCHER);
        }
        uint32_t alloc_time = osKernelGetTickCount() - alloc_start;
        
        // Free batch  
        uint32_t free_start = osKernelGetTickCount();
        for (int i = 0; i < 10; i++) {
            if (messages[i] != NULL) {
                MessagePool_Free(messages[i]);
                messages[i] = NULL;
            }
        }
        uint32_t free_time = osKernelGetTickCount() - free_start;
        
        if (iteration % 10 == 0) { // Print cada 10 iteraciones
            printf("Iteration %d: Alloc=%u ticks, Free=%u ticks\n", 
                   iteration, (unsigned int)alloc_time, (unsigned int)free_time);
        }
    }
    
    uint32_t total_time = osKernelGetTickCount() - start_time;
    printf("Performance Test: %u allocations in %u ticks\n", 
           (unsigned int)NUM_ALLOCATIONS, (unsigned int)total_time);
    
    printf("=== Performance Test Completed ===\n\n");
}

void MessageSystem_SensorExample(void) {
    printf("\n=== MessageSystem Sensor Example ===\n");
    
    // Simular datos de diferentes sensores
    typedef struct {
        float x, y, z;
        uint32_t timestamp;
    } ImuData_t;
    
    typedef struct {
        double latitude, longitude;
        float altitude;
        uint8_t satellites;
    } GpsData_t;
    
    // Crear mensaje IMU
    EmbeddedMessage_t* imu_msg = MessagePool_Allocate();
    if (imu_msg != NULL) {
        ImuData_t imu_data = {
            .x = 1.23f,
            .y = -2.45f, 
            .z = 9.81f,
            .timestamp = osKernelGetTickCount()
        };
        
        EmbeddedMessage_CreateWithPayload(imu_msg, MSG_ID_SEND_IMU,
                                        MODULE_SENSOR_ACQ, MODULE_DISPATCHER,
                                        (uint8_t*)&imu_data, sizeof(imu_data));
        
        printf("IMU Message: ID=%d, Length=%d bytes\n", imu_msg->id, imu_msg->length);
        
        // Simular lectura del payload
        if (imu_msg->length == sizeof(ImuData_t)) {
            ImuData_t* received_data = (ImuData_t*)imu_msg->payload;
            printf("IMU Data - X:%.2f Y:%.2f Z:%.2f Time:%u\n",
                   received_data->x, received_data->y, received_data->z,
                   (unsigned int)received_data->timestamp);
        }
        
        MessagePool_Free(imu_msg);
    }
    
    // Crear mensaje GPS
    EmbeddedMessage_t* gps_msg = MessagePool_Allocate();
    if (gps_msg != NULL) {
        GpsData_t gps_data = {
            .latitude = -34.6037,
            .longitude = -58.3816,
            .altitude = 25.0f,
            .satellites = 8
        };
        
        EmbeddedMessage_CreateWithPayload(gps_msg, MSG_ID_SEND_GPS,
                                        MODULE_GPS, MODULE_DISPATCHER,
                                        (uint8_t*)&gps_data, sizeof(gps_data));
        
        printf("GPS Message: ID=%d, Length=%d bytes\n", gps_msg->id, gps_msg->length);
        
        if (gps_msg->length == sizeof(GpsData_t)) {
            GpsData_t* received_data = (GpsData_t*)gps_msg->payload;
            printf("GPS Data - Lat:%.6f Lon:%.6f Alt:%.1f Sats:%d\n",
                   received_data->latitude, received_data->longitude, 
                   received_data->altitude, received_data->satellites);
        }
        
        MessagePool_Free(gps_msg);
    }
    
    printf("=== Sensor Example Completed ===\n\n");
}

void MessageSystem_StressTest(void) {
    printf("\n=== MessageSystem Stress Test ===\n");
    
    // Intentar llenar completamente el pool
    EmbeddedMessage_t* stress_messages[MESSAGE_POOL_SIZE + 5]; // +5 para testear overflow
    int allocated_count = 0;
    
    // Llenar el pool
    for (int i = 0; i < MESSAGE_POOL_SIZE + 5; i++) {
        stress_messages[i] = MessagePool_Allocate();
        if (stress_messages[i] != NULL) {
            allocated_count++;
            EmbeddedMessage_Create(stress_messages[i], MSG_ID_ACK,
                                 MODULE_DISPATCHER, MODULE_FSM);
        } else {
            printf("Pool exhausted at message %d (expected at %d)\n", 
                   i, MESSAGE_POOL_SIZE);
            break;
        }
    }
    
    printf("Successfully allocated %d messages\n", allocated_count);
    
    // Mostrar estadísticas en pool lleno
    uint32_t total, allocated, max_allocated;
    MessagePool_GetStats(&total, &allocated, &max_allocated);
    printf("Pool Stats (Full) - Total: %u, Allocated: %u, Max: %u\n", 
           (unsigned int)total, (unsigned int)allocated, (unsigned int)max_allocated);
    
    // Liberar la mitad
    int freed_count = 0;
    for (int i = 0; i < allocated_count; i += 2) {
        if (stress_messages[i] != NULL) {
            MessagePool_Free(stress_messages[i]);
            stress_messages[i] = NULL;
            freed_count++;
        }
    }
    
    printf("Freed %d messages\n", freed_count);
    
    // Intentar allocar nuevamente
    int reallocated = 0;
    for (int i = 0; i < freed_count; i++) {
        EmbeddedMessage_t* msg = MessagePool_Allocate();
        if (msg != NULL) {
            reallocated++;
            MessagePool_Free(msg); // Liberar inmediatamente
        }
    }
    
    printf("Successfully reallocated %d messages\n", reallocated);
    
    // Cleanup: liberar todos los mensajes restantes
    for (int i = 0; i < MESSAGE_POOL_SIZE + 5; i++) {
        if (stress_messages[i] != NULL) {
            MessagePool_Free(stress_messages[i]);
        }
    }
    
    // Verificar que el pool está limpio
    MessagePool_GetStats(&total, &allocated, &max_allocated);
    printf("Pool Stats (After cleanup) - Allocated: %u\n", (unsigned int)allocated);
    
    printf("=== Stress Test Completed ===\n\n");
}

// ====== Ejemplos C++ ======

#ifdef __cplusplus

void MessageSystem_CppWrapperExample(void) {
    printf("\n=== MessageWrapper C++ Example ===\n");
    
    // Crear mensaje usando wrapper
    MessageWrapper msg1(MSG_ID_SEND_GPS, MODULE_GPS, MODULE_DISPATCHER);
    
    if (msg1.isValid()) {
        printf("Created MessageWrapper: ID=%d, Sender=%d, Receiver=%d\n",
               msg1.getId(), msg1.getSender(), msg1.getReceiver());
        
        // Agregar payload usando wrapper
        uint8_t sample_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
        MessageResult_t result = msg1.setPayload(sample_data, sizeof(sample_data));
        
        if (result == MSG_RESULT_OK) {
            printf("Payload set successfully, length: %d\n", msg1.getLength());
            
            const uint8_t* payload = msg1.getPayload();
            printf("Payload content: ");
            for (int i = 0; i < msg1.getLength(); i++) {
                printf("0x%02X ", payload[i]);
            }
            printf("\n");
        }
    } else {
        printf("ERROR: Failed to create MessageWrapper\n");
    }
    
    // Test de copia usando wrapper
    MessageWrapper msg2 = msg1; // Copy constructor
    if (msg2.isValid()) {
        printf("Copy constructor successful\n");
        printf("Copied message: ID=%d, Length=%d\n", 
               msg2.getId(), msg2.getLength());
    }
    
    // Factory method
    MessageWrapper msg3 = MessageWrapper::createMessage(MSG_ID_SEND_IMU, 
                                                       MODULE_SENSOR_ACQ, 
                                                       MODULE_FSM);
    if (msg3.isValid()) {
        printf("Factory method successful: ID=%d\n", msg3.getId());
    }
    
    printf("=== C++ Wrapper Example Completed ===\n\n");
}

void MessageSystem_MigrationExample(void) {
    printf("\n=== Migration Example ===\n");
    
    // Ejemplo de cómo migrar desde Message* a MessageWrapper
    
    // Viejo estilo (simulado)
    printf("OLD Style: Message* msg = new Message(...);\n");
    
    // Nuevo estilo con wrapper
    MessageWrapper new_msg(MSG_ID_STIMULUS_VIBRATION_REQUEST, 
                          MODULE_FSM, MODULE_STIMULUS);
    
    if (new_msg.isValid()) {
        printf("NEW Style: MessageWrapper works!\n");
        
        // Para compatibilidad con colas que esperan Message*
        EmbeddedMessage_t* raw_msg = new_msg.getRawMessage();
        if (raw_msg != NULL) {
            printf("Raw message access for legacy code: ID=%d\n", raw_msg->id);
        }
        
        // Mostrar estadísticas del pool
        PrintMessagePoolStats();
    }
    
    printf("=== Migration Example Completed ===\n\n");
}

#endif
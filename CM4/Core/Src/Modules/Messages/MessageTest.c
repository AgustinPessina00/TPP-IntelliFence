#include "MessageTest.h"
#include <stdio.h>
#include <stdbool.h>

// Test simple del sistema de mensajes sin FreeRTOS
void test_message_pool_basic(void) {
    printf("\n=== BASIC MESSAGE POOL TEST (No FreeRTOS) ===\r\n");
    
    // Simulamos la inicialización sin mutex para test básico
    printf("1. Testing message allocation...\r\n");
    
    // Como no podemos usar el mutex sin FreeRTOS corriendo, 
    // hagamos un test manual de las estructuras
    EmbeddedMessage_t test_msg;
    
    // Test básico de crear mensaje
    MessageResult_t result = EmbeddedMessage_Create(&test_msg, MSG_ID_SEND_GPS, 
                                                   MODULE_GPS, MODULE_DISPATCHER);
    
    if (result == MSG_RESULT_OK) {
        printf("   Message creation successful\r\n");
        printf("     ID: %d, Sender: %d, Receiver: %d\r\n", 
               test_msg.id, test_msg.sender, test_msg.receiver);
    } else {
        printf("   Message creation failed: %d\r\n", result);
        return;
    }
    
    // Test de payload
    printf("2. Testing payload operations...\r\n");
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    
    result = EmbeddedMessage_SetPayload(&test_msg, test_data, sizeof(test_data));
    if (result == MSG_RESULT_OK) {
        printf("   Payload set successful, length: %d\r\n", test_msg.length);
        printf("   Payload data: ");
        for (int i = 0; i < test_msg.length; i++) {
            printf("0x%02X ", test_msg.payload[i]);
        }
        printf("\r\n");
    } else {
        printf("   Payload set failed: %d\r\n", result);
    }
    
    // Test de copy
    printf("3. Testing message copy...\r\n");
    EmbeddedMessage_t copy_msg;
    result = EmbeddedMessage_Copy(&copy_msg, &test_msg);
    
    if (result == MSG_RESULT_OK) {
        printf("   Message copy successful\r\n");
        printf("     Original ID: %d, Copy ID: %d\r\n", test_msg.id, copy_msg.id);
        printf("     Original length: %d, Copy length: %d\r\n", test_msg.length, copy_msg.length);
        
        // Verificar que el payload se copió correctamente
        bool payload_match = true;
        for (int i = 0; i < test_msg.length; i++) {
            if (test_msg.payload[i] != copy_msg.payload[i]) {
                payload_match = false;
                break;
            }
        }
        
        if (payload_match) {
            printf("   Payload copied correctly\r\n");
        } else {
            printf("   Payload copy mismatch\r\n");
        }
    } else {
        printf("   Message copy failed: %d\r\n", result);
    }
    
    // Test de diferentes tipos de mensajes
    printf("4. Testing different message types...\r\n");
    
    const struct {
        uint8_t msg_id;
        ModuleId_t sender;
        ModuleId_t receiver;
        const char* name;
    } test_cases[] = {
        {MSG_ID_SEND_IMU, MODULE_SENSOR_ACQ, MODULE_DISPATCHER, "IMU Sensor"},
        {MSG_ID_SEND_GPS, MODULE_GPS, MODULE_DISPATCHER, "GPS Data"},
        {MSG_ID_FENCE_UPDATE, MODULE_FENCE_UPDATE, MODULE_FSM, "Fence Update"},
        {MSG_ID_ACK, MODULE_DISPATCHER, MODULE_SENSOR_ACQ, "Acknowledgment"}
    };
    
    for (int i = 0; i < 4; i++) {
        EmbeddedMessage_t msg;
        result = EmbeddedMessage_Create(&msg, test_cases[i].msg_id, 
                                      test_cases[i].sender, test_cases[i].receiver);
        
        if (result == MSG_RESULT_OK) {
            printf("   %s message created (ID: 0x%02X)\r\n", 
                   test_cases[i].name, test_cases[i].msg_id);
        } else {
            printf("   %s message failed: %d\r\n", test_cases[i].name, result);
        }
    }
    
    // Test de límites de payload
    printf("5. Testing payload size limits...\r\n");
    
    // Test payload máximo
    uint8_t max_payload[MAX_MESSAGE_PAYLOAD_SIZE];
    for (int i = 0; i < MAX_MESSAGE_PAYLOAD_SIZE; i++) {
        max_payload[i] = i & 0xFF;
    }
    
    EmbeddedMessage_t max_msg;
    result = EmbeddedMessage_CreateWithPayload(&max_msg, MSG_ID_SEND_INA_MCU,
                                             MODULE_SENSOR_ACQ, MODULE_DISPATCHER,
                                             max_payload, MAX_MESSAGE_PAYLOAD_SIZE);
    
    if (result == MSG_RESULT_OK) {
        printf("   Max payload size (%d bytes) handled correctly\r\n", MAX_MESSAGE_PAYLOAD_SIZE);
    } else {
        printf("   Max payload test failed: %d\r\n", result);
    }
    
    // Test payload demasiado grande
    uint8_t oversized_payload[MAX_MESSAGE_PAYLOAD_SIZE + 1];
    EmbeddedMessage_t overflow_msg;
    result = EmbeddedMessage_CreateWithPayload(&overflow_msg, MSG_ID_SEND_INA_MCU,
                                             MODULE_SENSOR_ACQ, MODULE_DISPATCHER,
                                             oversized_payload, sizeof(oversized_payload));
    
    if (result == MSG_RESULT_ERROR_PAYLOAD_TOO_LARGE) {
        printf("   Oversized payload correctly rejected\r\n");
    } else {
        printf("   Oversized payload test failed: %d (expected %d)\r\n", 
               result, MSG_RESULT_ERROR_PAYLOAD_TOO_LARGE);
    }
    
    printf("\n=== BASIC TEST COMPLETED ===\r\n");
}

// Test completo con FreeRTOS (si está corriendo)
void run_message_system_test(void) {
    printf("\n*** MESSAGE SYSTEM FULL TEST ***\r\n");
    
    // Primero ejecutar test básico
    test_message_pool_basic();
    
    // Luego intentar inicializar el pool si FreeRTOS está corriendo
    printf("\n=== FREERTOS POOL TEST ===\r\n");
    
    MessageResult_t init_result = MessagePool_Init();
    if (init_result == MSG_RESULT_OK) {
        printf("MessagePool initialized successfully\r\n");
        
        // Test de allocate/free con pool real
        printf("Testing pool allocation...\r\n");
        
        EmbeddedMessage_t* msg1 = MessagePool_Allocate();
        EmbeddedMessage_t* msg2 = MessagePool_Allocate();
        EmbeddedMessage_t* msg3 = MessagePool_Allocate();
        
        if (msg1 && msg2 && msg3) {
            printf("Multiple allocations successful\r\n");
            
            // Crear mensajes diferentes
            EmbeddedMessage_Create(msg1, MSG_ID_SEND_GPS, MODULE_GPS, MODULE_DISPATCHER);
            EmbeddedMessage_Create(msg2, MSG_ID_SEND_IMU, MODULE_SENSOR_ACQ, MODULE_DISPATCHER);
            EmbeddedMessage_Create(msg3, MSG_ID_ACK, MODULE_DISPATCHER, MODULE_FSM);
            
            printf("Messages created with IDs: %d, %d, %d\r\n", 
                   msg1->id, msg2->id, msg3->id);
            
            // Test de estadísticas
            uint32_t total, allocated, max_allocated;
            MessagePool_GetStats(&total, &allocated, &max_allocated);
            printf("Pool Stats - Total: %u, Allocated: %u, Max: %u\r\n", 
                   (unsigned int)total, (unsigned int)allocated, (unsigned int)max_allocated);
            
            // Liberar mensajes
            MessagePool_Free(msg1);
            MessagePool_Free(msg2);
            MessagePool_Free(msg3);
            
            printf("Messages freed successfully\r\n");
            
            // Verificar estadísticas después de liberación
            MessagePool_GetStats(&total, &allocated, &max_allocated);
            printf("Pool Stats (after free) - Allocated: %u\r\n", (unsigned int)allocated);
            
        } else {
            printf("Pool allocation failed\r\n");
        }
    } else {
        printf("MessagePool initialization failed: %d\r\n", init_result);
        printf("  (This is normal if FreeRTOS is not running yet)\r\n");
    }
    
    printf("\n*** MESSAGE SYSTEM TEST COMPLETED ***\n\r\n");
}
#ifndef MESSAGE_TEST_H
#define MESSAGE_TEST_H

#include "EmbeddedMessage.h"

#ifdef __cplusplus
extern "C" {
#endif

// Test simple que se puede llamar desde main.c
void run_message_system_test(void);

// Test individual que se puede llamar sin FreeRTOS
void test_message_pool_basic(void);

#ifdef __cplusplus
}
#endif

#endif // MESSAGE_TEST_H
#ifndef MESSAGE_EXAMPLE_H
#define MESSAGE_EXAMPLE_H

#include "EmbeddedMessage.h"
#include "MessageWrapper.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// ====== Funciones de Ejemplo y Test ======

// Test básico del sistema de mensajes
void MessageSystem_BasicTest(void);

// Test de performance del pool
void MessageSystem_PerformanceTest(void);

// Ejemplo de uso con sensores
void MessageSystem_SensorExample(void);

// Test de stress del pool
void MessageSystem_StressTest(void);

#ifdef __cplusplus
}

// ====== Ejemplos en C++ ======

// Ejemplo usando MessageWrapper
void MessageSystem_CppWrapperExample(void);

// Ejemplo de migración gradual
void MessageSystem_MigrationExample(void);

#endif

#endif // MESSAGE_EXAMPLE_H
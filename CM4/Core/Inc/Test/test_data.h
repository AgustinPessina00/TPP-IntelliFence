#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "Types/cow.h"
#include "Types/fence.h"
#include "Types/zone.h"
#include <stdint.h>

// ============================================================================
// TEST DATA STRUCTURES FOR FSM TESTING
// ============================================================================

/**
 * @brief Estructura de datos de prueba del GPS
 */
typedef struct {
    Position position;
    zone_t expectedZone;
    float distanceToLimit;  // en metros
    const char* description; // Descripción del escenario
} TestGPSData_t;

/**
 * @brief Estructura de datos de prueba del IMU
 */
typedef struct {
    Acceleration acceleration;
    CowState expectedState;
    const char* description;
} TestIMUData_t;

/**
 * @brief Estructura de respuesta de zona
 */
typedef struct {
    zone_t zone;
    float distance;
} TestZoneData_t;

// ============================================================================
// ESCENARIOS DE PRUEBA
// ============================================================================

/**
 * ESCENARIO 1: STARTUP ROUTINE
 * - La vaca está en GREEN_ZONE, estado GRAZING
 * - Se establece la cerca y se inicia operación normal
 * 
 * ESCENARIO 2: OPERACIÓN NORMAL - GREEN ZONE
 * - Vaca pastando (GRAZING) en zona segura
 * - Vaca durmiendo (SLEEP)
 * - Vaca moviéndose (MOVEMENT) pero dentro de zona verde
 * 
 * ESCENARIO 3: APROXIMACIÓN AL LÍMITE
 * - Vaca se mueve desde GREEN → LIGHT_BLUE → BLUE → DARK_BLUE
 * - Debe activarse el buzzer progresivamente
 * 
 * ESCENARIO 4: ZONA DE ESTÍMULO
 * - Vaca entra en YELLOW_ZONE (buzzer + vibración)
 * - Vaca continúa a RED_ZONE (vibración intensa)
 * 
 * ESCENARIO 5: ESCAPE
 * - Vaca llega a BLACK_ZONE (se escapó)
 * - Alerta crítica
 * 
 * ESCENARIO 6: REGRESO
 * - Vaca regresa desde zona de estímulo a GREEN_ZONE
 */

// ============================================================================
// DATOS DE CERCA DE PRUEBA
// ============================================================================

// Cerca cuadrada centrada en (0, 0) para facilitar los cálculos
// Dimensiones: ~222m x 222m (equivalente a ~100m de radio)
#define TEST_FENCE_VERTEX_COUNT 4
extern const Vertex TEST_FENCE_VERTICES[TEST_FENCE_VERTEX_COUNT];

// ============================================================================
// DATOS DE GPS DE PRUEBA
// ============================================================================

#define TEST_GPS_DATA_COUNT 26
extern const TestGPSData_t TEST_GPS_DATA[TEST_GPS_DATA_COUNT];

// ============================================================================
// DATOS DE IMU DE PRUEBA
// ============================================================================

#define TEST_IMU_DATA_COUNT 30
extern const TestIMUData_t TEST_IMU_DATA[TEST_IMU_DATA_COUNT];

// ============================================================================
// TEST CONTROLLER
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializar el sistema de test
 * @note Debe llamarse antes de iniciar la FSM
 */
void TestData_Init(void);

/**
 * @brief Obtener el siguiente dato de GPS
 * @return Puntero al dato de GPS actual (NULL si no hay más datos)
 */
const TestGPSData_t* TestData_GetNextGPS(void);

/**
 * @brief Obtener el siguiente dato de IMU
 * @return Puntero al dato de IMU actual (NULL si no hay más datos)
 */
const TestIMUData_t* TestData_GetNextIMU(void);

/**
 * @brief Obtener datos de zona basados en la posición actual
 * @param position Posición de la vaca
 * @return Datos de zona calculados
 */
TestZoneData_t TestData_GetZone(Position position);

/**
 * @brief Resetear los índices de datos de prueba
 */
void TestData_Reset(void);

/**
 * @brief Obtener el índice actual de GPS
 */
uint32_t TestData_GetGPSIndex(void);

/**
 * @brief Obtener el índice actual de IMU
 */
uint32_t TestData_GetIMUIndex(void);

/**
 * @brief Verificar si quedan datos de prueba
 */
bool TestData_HasMoreData(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DATA_H

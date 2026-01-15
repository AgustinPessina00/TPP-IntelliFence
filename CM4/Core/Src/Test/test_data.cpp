#include "Test/test_data.h"
#include <math.h>

// ============================================================================
// ÍNDICES DE DATOS DE PRUEBA
// ============================================================================

static uint32_t gpsIndex = 0;
static uint32_t imuIndex = 0;

// ============================================================================
// CERCA DE PRUEBA - Polígono en Flores, Buenos Aires
// ============================================================================

// Centro aproximado en (-34.5633, -58.4612) - Flores, Buenos Aires
// Polígono de 4 vértices con perímetro de ~800m
// Umbrales: 80m (LIGHT_BLUE), 60m (BLUE), 40m (DARK_BLUE), 20m (YELLOW), 0m (RED)
const Vertex TEST_FENCE_VERTICES[TEST_FENCE_VERTEX_COUNT] = {
    {-34.561846f, -58.461113f},  // Vértice 1
    {-34.562827f, -58.462862f},  // Vértice 2
    {-34.564726f, -58.461292f},  // Vértice 3
    {-34.563648f, -58.459404f}   // Vértice 4
};

// ============================================================================
// DATOS DE GPS - 26 POSICIONES DEL RECORRIDO REAL EN FLORES
// ============================================================================

const TestGPSData_t TEST_GPS_DATA[TEST_GPS_DATA_COUNT] = {
    // Puntos 1-4: Zona Segura (>85m)
    {{-34.563363f, -58.461126f}, GREEN_ZONE, 97.9f, "GREEN: Zona segura - 97.9m"},
    {{-34.563248f, -58.461244f}, GREEN_ZONE, 96.2f, "GREEN: Zona segura - 96.2m"},
    {{-34.563089f, -58.461383f}, GREEN_ZONE, 95.6f, "GREEN: Zona segura - 95.6m"},
    {{-34.563031f, -58.461458f}, GREEN_ZONE, 91.1f, "GREEN: Zona segura - 91.1m"},
    
    // Punto 5: Zona Precaución (70-85m)
    {{-34.562939f, -58.461560f}, LIGHT_BLUE_ZONE, 77.4f, "LIGHT_BLUE: Precaución - 77.4m"},
    
    // Puntos 6-7: Zona Alerta (50-70m)
    {{-34.562846f, -58.461672f}, BLUE_ZONE, 63.0f, "BLUE: Alerta - 63.0m"},
    {{-34.562841f, -58.461801f}, BLUE_ZONE, 56.0f, "BLUE: Alerta - 56.0m"},
    
    // Puntos 8-9: Zona Advertencia (30-50m)
    {{-34.562766f, -58.461935f}, DARK_BLUE_ZONE, 42.2f, "DARK_BLUE: Advertencia - 42.2m"},
    {{-34.562749f, -58.462085f}, DARK_BLUE_ZONE, 32.8f, "DARK_BLUE: Advertencia - 32.8m"},
    
    // Puntos 10-11: Zona Peligro (10-30m)
    {{-34.562643f, -58.462187f}, YELLOW_ZONE, 17.8f, "YELLOW: Peligro - 17.8m"},
    {{-34.562532f, -58.461962f}, YELLOW_ZONE, 19.3f, "YELLOW: Peligro - 19.3m"},
    
    // Punto 12: Zona Crítica (0-10m)
    {{-34.562452f, -58.462096f}, RED_ZONE, 5.0f, "RED: Crítica - 5.0m"},
    
    // Puntos 13-14: ESCAPE (Fuera del cerco)
    {{-34.562262f, -58.462289f}, BLACK_ZONE, -15.0f, "BLACK: Fuera del cerco - ESCAPE"},
    {{-34.562430f, -58.462477f}, BLACK_ZONE, -20.0f, "BLACK: Fuera del cerco - ESCAPE"},
    
    // Puntos 15-16: Regreso - Zona Crítica (0-10m)
    {{-34.562669f, -58.462503f}, RED_ZONE, 3.9f, "RED: Regreso - Crítica - 3.9m"},
    {{-34.562691f, -58.462482f}, RED_ZONE, 7.1f, "RED: Crítica - 7.1m"},
    
    // Puntos 17-18: Zona Peligro (10-30m)
    {{-34.562744f, -58.462439f}, YELLOW_ZONE, 14.2f, "YELLOW: Peligro - 14.2m"},
    {{-34.562780f, -58.462386f}, YELLOW_ZONE, 20.2f, "YELLOW: Peligro - 20.2m"},
    
    // Puntos 19-20: Zona Advertencia (30-50m)
    {{-34.562868f, -58.462241f}, DARK_BLUE_ZONE, 35.8f, "DARK_BLUE: Advertencia - 35.8m"},
    {{-34.562925f, -58.462187f}, DARK_BLUE_ZONE, 43.8f, "DARK_BLUE: Advertencia - 43.8m"},
    
    // Puntos 21-22: Zona Alerta (50-70m)
    {{-34.563014f, -58.461973f}, BLUE_ZONE, 55.7f, "BLUE: Alerta - 55.7m"},
    {{-34.563067f, -58.461849f}, BLUE_ZONE, 61.7f, "BLUE: Alerta - 61.7m"},
    
    // Puntos 23-25: Zona Precaución (70-85m)
    {{-34.563098f, -58.461656f}, LIGHT_BLUE_ZONE, 74.4f, "LIGHT_BLUE: Precaución - 74.4m"},
    {{-34.563124f, -58.461613f}, LIGHT_BLUE_ZONE, 75.9f, "LIGHT_BLUE: Precaución - 75.9m"},
    {{-34.563186f, -58.461571f}, LIGHT_BLUE_ZONE, 75.3f, "LIGHT_BLUE: Precaución - 75.3m"},
    
    // Punto 26: Zona Segura (>85m)
    {{-34.563182f, -58.461362f}, GREEN_ZONE, 91.4f, "GREEN: Zona segura - 91.4m"}
};

// ============================================================================
// DATOS DE IMU - 30 MUESTRAS DE ACELERACIÓN
// ============================================================================

const TestIMUData_t TEST_IMU_DATA[TEST_IMU_DATA_COUNT] = {
    // ESCENARIO 1: STARTUP - Vaca quieta al inicio
    {{0.02, -0.01, 9.81}, CowState::SLEEP, "STARTUP: Vaca en reposo"},
    
    // ESCENARIO 2: GRAZING - Pastando (movimientos suaves de cabeza)
    {{0.15, 0.12, 9.85}, CowState::GRAZING, "GRAZING: Pastando tranquila"},
    {{0.18, -0.10, 9.78}, CowState::GRAZING, "GRAZING: Comiendo pasto"},
    {{0.12, 0.08, 9.82}, CowState::GRAZING, "GRAZING: Movimientos lentos"},
    {{0.20, 0.15, 9.80}, CowState::GRAZING, "GRAZING: Masticando"},
    
    // ESCENARIO 3: SLEEP - Durmiendo (casi sin movimiento)
    {{0.01, 0.02, 9.81}, CowState::SLEEP, "SLEEP: Durmiendo profundo"},
    {{-0.01, 0.01, 9.80}, CowState::SLEEP, "SLEEP: Respiración lenta"},
    {{0.02, -0.01, 9.82}, CowState::SLEEP, "SLEEP: En reposo"},
    
    // ESCENARIO 4: MOVEMENT - Caminando lento
    {{0.45, 0.38, 10.20}, CowState::MOVEMENT, "MOVEMENT: Caminando lento"},
    {{0.52, -0.42, 10.35}, CowState::MOVEMENT, "MOVEMENT: Avanzando"},
    {{0.48, 0.40, 10.15}, CowState::MOVEMENT, "MOVEMENT: Explorando"},
    
    // ESCENARIO 5: MOVEMENT - Caminando normal (acercándose al límite)
    {{0.65, 0.55, 10.50}, CowState::MOVEMENT, "MOVEMENT: Caminata normal"},
    {{0.72, -0.60, 10.65}, CowState::MOVEMENT, "MOVEMENT: Paso firme"},
    {{0.68, 0.58, 10.48}, CowState::MOVEMENT, "MOVEMENT: Avance constante"},
    
    // ESCENARIO 6: MOVEMENT - Movimiento rápido (con buzzer)
    {{0.85, 0.75, 11.00}, CowState::MOVEMENT, "MOVEMENT: Caminando rápido"},
    {{0.92, -0.80, 11.20}, CowState::MOVEMENT, "MOVEMENT: Molesta por buzzer"},
    {{0.88, 0.78, 11.10}, CowState::MOVEMENT, "MOVEMENT: Intenta seguir"},
    
    // ESCENARIO 7: MOVEMENT - Intensa (con buzzer + vibración)
    {{1.10, 0.95, 11.50}, CowState::MOVEMENT, "MOVEMENT: Movimiento intenso"},
    {{1.25, -1.05, 11.80}, CowState::MOVEMENT, "MOVEMENT: Agitada por estímulo"},
    {{1.15, 1.00, 11.60}, CowState::MOVEMENT, "MOVEMENT: Incomodidad notable"},
    
    // ESCENARIO 8: MOVEMENT - Muy intensa (vibración fuerte)
    {{1.45, 1.25, 12.20}, CowState::MOVEMENT, "MOVEMENT: Muy agitada"},
    {{1.60, -1.40, 12.50}, CowState::MOVEMENT, "MOVEMENT: Tratando de escapar"},
    {{1.55, 1.35, 12.35}, CowState::MOVEMENT, "MOVEMENT: Resistiendo estímulo"},
    
    // ESCENARIO 9: MOVEMENT - Escape (movimiento brusco)
    {{1.85, 1.65, 13.00}, CowState::MOVEMENT, "MOVEMENT: ESCAPE - Corriendo"},
    {{2.00, -1.80, 13.40}, CowState::MOVEMENT, "MOVEMENT: Fuera del cerco"},
    
    // ESCENARIO 10: REGRESO - Vuelve caminando
    {{1.20, 1.05, 11.70}, CowState::MOVEMENT, "MOVEMENT: Regresando"},
    {{0.75, 0.62, 10.60}, CowState::MOVEMENT, "MOVEMENT: Volviendo"},
    {{0.50, -0.45, 10.25}, CowState::MOVEMENT, "MOVEMENT: Caminata tranquila"},
    {{0.18, 0.12, 9.85}, CowState::GRAZING, "GRAZING: De vuelta pastando"},
    {{0.02, -0.01, 9.81}, CowState::SLEEP, "SLEEP: Descansando después"}
};

// ============================================================================
// FUNCIONES DE CONTROL
// ============================================================================

void TestData_Init(void) {
    gpsIndex = 0;
    imuIndex = 0;
}

void TestData_Reset(void) {
    gpsIndex = 0;
    imuIndex = 0;
}

const TestGPSData_t* TestData_GetNextGPS(void) {
    if (gpsIndex >= TEST_GPS_DATA_COUNT) {
        return nullptr;  // No hay más datos
    }
    return &TEST_GPS_DATA[gpsIndex++];
}

const TestIMUData_t* TestData_GetNextIMU(void) {
    if (imuIndex >= TEST_IMU_DATA_COUNT) {
        return nullptr;  // No hay más datos
    }
    return &TEST_IMU_DATA[imuIndex++];
}

TestZoneData_t TestData_GetZone(Position position) {
    TestZoneData_t result;
    
    // Por simplicidad, usar el dato pre-calculado del índice GPS actual
    if (gpsIndex > 0 && gpsIndex <= TEST_GPS_DATA_COUNT) {
        const TestGPSData_t* gpsData = &TEST_GPS_DATA[gpsIndex - 1];
        result.zone = gpsData->expectedZone;
        result.distance = gpsData->distanceToLimit;
    } else {
        result.zone = GREEN_ZONE;
        result.distance = 0.0f;
    }
    
    return result;
}

uint32_t TestData_GetGPSIndex(void) {
    return gpsIndex;
}

uint32_t TestData_GetIMUIndex(void) {
    return imuIndex;
}

bool TestData_HasMoreData(void) {
    return (gpsIndex < TEST_GPS_DATA_COUNT) || (imuIndex < TEST_IMU_DATA_COUNT);
}

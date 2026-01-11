#include "Test/test_data.h"
#include <math.h>

// ============================================================================
// ÍNDICES DE DATOS DE PRUEBA
// ============================================================================

static uint32_t gpsIndex = 0;
static uint32_t imuIndex = 0;

// ============================================================================
// CERCA DE PRUEBA - Cuadrado de ~222m x 222m
// ============================================================================

// Centro en (-34.9205, -57.9536) - Coordenadas ejemplo cerca de La Plata
// Cada 0.001° ≈ 111m de latitud
// Radio de ~100m = 0.001° en cada dirección
const Vertex TEST_FENCE_VERTICES[TEST_FENCE_VERTEX_COUNT] = {
    {-34.919500, -57.954600},  // NO (Noroeste)
    {-34.919500, -57.952600},  // NE (Noreste)
    {-34.921500, -57.952600},  // SE (Sureste)
    {-34.921500, -57.954600}   // SO (Suroeste)
};

// ============================================================================
// DATOS DE GPS - 30 POSICIONES QUE RECORREN TODAS LAS ZONAS
// ============================================================================

const TestGPSData_t TEST_GPS_DATA[TEST_GPS_DATA_COUNT] = {
    // ESCENARIO 1: STARTUP - Posición inicial en GREEN_ZONE
    {{-34.920400, -57.953600}, GREEN_ZONE, 5.0, "STARTUP: Centro del cerco"},
    
    // ESCENARIO 2: OPERACIÓN NORMAL - GREEN ZONE (pastando/durmiendo)
    {{-34.920350, -57.953580}, GREEN_ZONE, 8.0, "GREEN: Pastando tranquila"},
    {{-34.920320, -57.953550}, GREEN_ZONE, 12.0, "GREEN: Durmiendo"},
    {{-34.920380, -57.953620}, GREEN_ZONE, 6.5, "GREEN: Movimiento lento"},
    {{-34.920420, -57.953650}, GREEN_ZONE, 4.2, "GREEN: Cerca del centro"},
    
    // ESCENARIO 3: MOVIMIENTO DENTRO DE GREEN_ZONE
    {{-34.920450, -57.953700}, GREEN_ZONE, 8.5, "GREEN: Explorando"},
    {{-34.920480, -57.953750}, GREEN_ZONE, 15.0, "GREEN: Alejándose"},
    {{-34.920500, -57.953800}, GREEN_ZONE, 22.0, "GREEN: Más lejos"},
    
    // ESCENARIO 4: APROXIMACIÓN AL LÍMITE - LIGHT_BLUE_ZONE
    {{-34.920520, -57.953850}, LIGHT_BLUE_ZONE, 35.0, "LIGHT_BLUE: Buzzer leve"},
    {{-34.920530, -57.953880}, LIGHT_BLUE_ZONE, 42.0, "LIGHT_BLUE: Acercándose"},
    {{-34.920540, -57.953900}, LIGHT_BLUE_ZONE, 48.0, "LIGHT_BLUE: Sigue avanzando"},
    
    // ESCENARIO 5: BLUE_ZONE - Buzzer medio
    {{-34.920550, -57.953930}, BLUE_ZONE, 55.0, "BLUE: Buzzer medio"},
    {{-34.920560, -57.953950}, BLUE_ZONE, 62.0, "BLUE: Advertencia aumenta"},
    {{-34.920570, -57.953970}, BLUE_ZONE, 68.0, "BLUE: Cerca del límite azul"},
    
    // ESCENARIO 6: DARK_BLUE_ZONE - Buzzer intenso
    {{-34.920580, -57.953990}, DARK_BLUE_ZONE, 75.0, "DARK_BLUE: Buzzer intenso"},
    {{-34.920590, -57.954010}, DARK_BLUE_ZONE, 82.0, "DARK_BLUE: Muy cerca del límite"},
    {{-34.920595, -57.954020}, DARK_BLUE_ZONE, 88.0, "DARK_BLUE: Peligro inminente"},
    
    // ESCENARIO 7: YELLOW_ZONE - Buzzer + Vibración
    {{-34.920600, -57.954030}, YELLOW_ZONE, 95.0, "YELLOW: Buzzer + Vibración"},
    {{-34.920605, -57.954040}, YELLOW_ZONE, 102.0, "YELLOW: Estímulo fuerte"},
    {{-34.920610, -57.954050}, YELLOW_ZONE, 108.0, "YELLOW: Debe retroceder"},
    
    // ESCENARIO 8: RED_ZONE - Vibración intensa
    {{-34.920615, -57.954060}, RED_ZONE, 115.0, "RED: Vibración intensa"},
    {{-34.920620, -57.954070}, RED_ZONE, 122.0, "RED: Última advertencia"},
    {{-34.920623, -57.954075}, RED_ZONE, 128.0, "RED: Al borde del escape"},
    
    // ESCENARIO 9: BLACK_ZONE - Se escapó
    {{-34.920628, -57.954085}, BLACK_ZONE, 138.0, "BLACK: ESCAPE - Alerta crítica"},
    {{-34.920632, -57.954090}, BLACK_ZONE, 145.0, "BLACK: Fuera del cerco"},
    
    // ESCENARIO 10: REGRESO - Vuelve hacia zona segura
    {{-34.920625, -57.954078}, RED_ZONE, 130.0, "RED: Regresando"},
    {{-34.920610, -57.954050}, YELLOW_ZONE, 110.0, "YELLOW: Volviendo"},
    {{-34.920580, -57.953990}, DARK_BLUE_ZONE, 78.0, "DARK_BLUE: Retorno exitoso"},
    {{-34.920500, -57.953800}, GREEN_ZONE, 25.0, "GREEN: De vuelta en zona segura"},
    {{-34.920400, -57.953600}, GREEN_ZONE, 5.0, "GREEN: Centro - Test completo"}
};

// ============================================================================
// DATOS DE IMU - 30 MUESTRAS DE ACELERACIÓN
// ============================================================================

const TestIMUData_t TEST_IMU_DATA[TEST_IMU_DATA_COUNT] = {
    // ESCENARIO 1: STARTUP - Vaca quieta al inicio
    {{0.02, -0.01, 9.81}, SLEEP, "STARTUP: Vaca en reposo"},
    
    // ESCENARIO 2: GRAZING - Pastando (movimientos suaves de cabeza)
    {{0.15, 0.12, 9.85}, GRAZING, "GRAZING: Pastando tranquila"},
    {{0.18, -0.10, 9.78}, GRAZING, "GRAZING: Comiendo pasto"},
    {{0.12, 0.08, 9.82}, GRAZING, "GRAZING: Movimientos lentos"},
    {{0.20, 0.15, 9.80}, GRAZING, "GRAZING: Masticando"},
    
    // ESCENARIO 3: SLEEP - Durmiendo (casi sin movimiento)
    {{0.01, 0.02, 9.81}, SLEEP, "SLEEP: Durmiendo profundo"},
    {{-0.01, 0.01, 9.80}, SLEEP, "SLEEP: Respiración lenta"},
    {{0.02, -0.01, 9.82}, SLEEP, "SLEEP: En reposo"},
    
    // ESCENARIO 4: MOVEMENT - Caminando lento
    {{0.45, 0.38, 10.20}, MOVEMENT, "MOVEMENT: Caminando lento"},
    {{0.52, -0.42, 10.35}, MOVEMENT, "MOVEMENT: Avanzando"},
    {{0.48, 0.40, 10.15}, MOVEMENT, "MOVEMENT: Explorando"},
    
    // ESCENARIO 5: MOVEMENT - Caminando normal (acercándose al límite)
    {{0.65, 0.55, 10.50}, MOVEMENT, "MOVEMENT: Caminata normal"},
    {{0.72, -0.60, 10.65}, MOVEMENT, "MOVEMENT: Paso firme"},
    {{0.68, 0.58, 10.48}, MOVEMENT, "MOVEMENT: Avance constante"},
    
    // ESCENARIO 6: MOVEMENT - Movimiento rápido (con buzzer)
    {{0.85, 0.75, 11.00}, MOVEMENT, "MOVEMENT: Caminando rápido"},
    {{0.92, -0.80, 11.20}, MOVEMENT, "MOVEMENT: Molesta por buzzer"},
    {{0.88, 0.78, 11.10}, MOVEMENT, "MOVEMENT: Intenta seguir"},
    
    // ESCENARIO 7: MOVEMENT - Intensa (con buzzer + vibración)
    {{1.10, 0.95, 11.50}, MOVEMENT, "MOVEMENT: Movimiento intenso"},
    {{1.25, -1.05, 11.80}, MOVEMENT, "MOVEMENT: Agitada por estímulo"},
    {{1.15, 1.00, 11.60}, MOVEMENT, "MOVEMENT: Incomodidad notable"},
    
    // ESCENARIO 8: MOVEMENT - Muy intensa (vibración fuerte)
    {{1.45, 1.25, 12.20}, MOVEMENT, "MOVEMENT: Muy agitada"},
    {{1.60, -1.40, 12.50}, MOVEMENT, "MOVEMENT: Tratando de escapar"},
    {{1.55, 1.35, 12.35}, MOVEMENT, "MOVEMENT: Resistiendo estímulo"},
    
    // ESCENARIO 9: MOVEMENT - Escape (movimiento brusco)
    {{1.85, 1.65, 13.00}, MOVEMENT, "MOVEMENT: ESCAPE - Corriendo"},
    {{2.00, -1.80, 13.40}, MOVEMENT, "MOVEMENT: Fuera del cerco"},
    
    // ESCENARIO 10: REGRESO - Vuelve caminando
    {{1.20, 1.05, 11.70}, MOVEMENT, "MOVEMENT: Regresando"},
    {{0.75, 0.62, 10.60}, MOVEMENT, "MOVEMENT: Volviendo"},
    {{0.50, -0.45, 10.25}, MOVEMENT, "MOVEMENT: Caminata tranquila"},
    {{0.18, 0.12, 9.85}, GRAZING, "GRAZING: De vuelta pastando"},
    {{0.02, -0.01, 9.81}, SLEEP, "SLEEP: Descansando después"}
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
        result.distance = 0.0;
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

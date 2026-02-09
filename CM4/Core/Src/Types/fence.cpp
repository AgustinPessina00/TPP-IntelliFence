/**
 * @file fence.cpp
 * @brief Embedded-friendly Fence implementation without STL (no dynamic allocation)
 * @author TPP-IntelliFence Team
 * @date 2025
 */

#include "fence.h"
#include <string.h> // Para memcpy

// ============================================================================
// CONSTRUCTOR
// ============================================================================

Fence::Fence() : limitCount(0), isInitialized(false) {
    init();
}

void Fence::init() {
    limitCount = 0;
    
    // Inicializar umbrales de zona (en metros desde el límite)
    // Ajustados para fence en Flores, Buenos Aires con datos de campo
    // GREEN_ZONE (Segura): 85m+
    // LIGHT_BLUE_ZONE (Precaución): 70-85m
    // BLUE_ZONE (Alerta): 50-70m
    // DARK_BLUE_ZONE (Advertencia): 30-50m
    // YELLOW_ZONE (Peligro): 10-30m
    // RED_ZONE (Crítica): 0-10m
    this->lightBlue = 25.0f;  // LIGHT_BLUE_ZONE: 70-85m desde límite
    this->blue = 20.0f;       // BLUE_ZONE: 50-70m desde límite
    this->darkBlue = 15.0f;   // DARK_BLUE_ZONE: 30-50m desde límite
    this->yellow = 10.0f;     // YELLOW_ZONE: 10-30m desde límite
    this->red = 5.0f;        // RED_ZONE: 0-10m desde límite

    setZoneThresholds();

    // Inicializar centro del cerco
    this->centerFence = {0.0f, 0.0f};
    
    // Inicializar array a cero
    memset(limites, 0, sizeof(limites));

    this->hasValidFence = false;
    
    this->isInitialized = true;
}

// ============================================================================
// LIMIT MANAGEMENT
// ============================================================================

void Fence::createLimits(const Vertex* v, uint8_t count) {
    if (!isInitialized) {
        printf("[FENCE] ERROR: Fence not initialized\r\r\n");
        return;
    }
    
    limitCount = 0;
    
    // Validar entrada
    if (v == nullptr) {
        printf("[FENCE] ERROR: Null vertex pointer\r\r\n");
        return;
    }
    
    if (count > MAX_VERTICES) {
        printf("[FENCE] ERROR: Vertex count %d exceeds MAX_VERTICES %d\r\r\n", count, MAX_VERTICES);
        return;
    }
    
    if (count < 2) {
        printf("[FENCE] WARNING: Need at least 2 vertices to create limits (have %d)\r\r\n", count);
        return;
    }

    // Crear líneas conectando vértices consecutivos
    for (uint8_t i = 0; i < count; i++) {
        limites[i].start = v[i];
        limites[i].end = v[(i + 1) % count];  // Cierre del polígono
        limitCount++;
    }

    // Actualizar centro del cerco usando los vértices recibidos
    updateCenterFence(v, count);

    this->hasValidFence = true;
    
    printf("[FENCE] Created %d limits from %d vertices\r\r\n", limitCount, count);
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void Fence::updateCenterFence(const Vertex* v, uint8_t count) {
    if (count == 0 || v == nullptr) {
        centerFence = {0.0f, 0.0f};
        return;
    }

    float latSum = 0.0f;
    float lonSum = 0.0f;

    // Calcular promedio de coordenadas
    for (uint8_t i = 0; i < count; i++) {
        latSum += v[i].latitude;
        lonSum += v[i].longitude;
    }

    centerFence.latitude = latSum / count;
    centerFence.longitude = lonSum / count;
    
    printf("[FENCE] Center updated: (%.6f, %.6f)\r\r\n", centerFence.latitude, centerFence.longitude);
}

// ============================================================================
// ZONE THRESHOLDS
// ============================================================================

void Fence::setZoneThresholds() {
    // Verificar que los umbrales estén en orden lógico (decreciente)
    if (!(lightBlue > blue && blue > darkBlue && darkBlue > yellow && yellow > red)) {
        printf("[FENCE] ERROR: Thresholds must be in descending order\r\r\n");
        return;
    }

    // Asignar umbrales al array público (índices basados en 0)
    // zone_t values: LIGHT_BLUE=1, BLUE=2, DARK_BLUE=3, YELLOW=4, RED=5
    // Array indices: [0]=LIGHT_BLUE, [1]=BLUE, [2]=DARK_BLUE, [3]=YELLOW, [4]=RED
    this->thresholds[0] = this->lightBlue;  // LIGHT_BLUE_ZONE
    this->thresholds[1] = this->blue;       // BLUE_ZONE
    this->thresholds[2] = this->darkBlue;   // DARK_BLUE_ZONE
    this->thresholds[3] = this->yellow;     // YELLOW_ZONE
    this->thresholds[4] = this->red;        // RED_ZONE
    
    printf("[FENCE] Thresholds set: LIGHT_BLUE=%.1fm, BLUE=%.1fm, DARK_BLUE=%.1fm, YELLOW=%.1fm, RED=%.1fm\r\r\n",
           lightBlue, blue, darkBlue, yellow, red);
}

float Fence::getThreshold(zone_t zone) const {
    // Convertir zone_t enum a índice de array (zone: 1-5 → index: 0-4)
    if (zone >= LIGHT_BLUE_ZONE && zone <= RED_ZONE) {
        return thresholds[zone - 1];  // zone 1 → index 0, zone 5 → index 4
    }
    
    printf("[FENCE] WARNING: Invalid zone %d\r\r\n", zone);
    return -1.0f;  // No aplica
}

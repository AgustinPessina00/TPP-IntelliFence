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

Fence::Fence() : vertexCount(0), limitCount(0) {
    // Inicializar umbrales de zona (en metros desde el límite)
    this->lightBlue = 20.0f;  // LIGHT_BLUE_ZONE: 15-20m desde límite
    this->blue = 15.0f;       // BLUE_ZONE: 10-15m desde límite
    this->darkBlue = 10.0f;   // DARK_BLUE_ZONE: 5-10m desde límite
    this->yellow = 5.0f;      // YELLOW_ZONE: 1-5m desde límite
    this->red = 1.0f;         // RED_ZONE: 0-1m desde límite

    setZoneThresholds();

    // Inicializar centro del cerco
    this->centerFence = {0.0, 0.0};
    
    // Inicializar arrays a cero
    memset(vertices, 0, sizeof(vertices));
    memset(limites, 0, sizeof(limites));
}

// ============================================================================
// VERTEX MANAGEMENT
// ============================================================================

bool Fence::saveVertices(const Vertex* v, uint8_t count) {
    // Validar que no exceda la capacidad máxima
    if (count > MAX_VERTICES) {
        printf("[FENCE] ERROR: Vertex count %d exceeds MAX_VERTICES %d\r\n", count, MAX_VERTICES);
        return false;
    }
    
    // Validar puntero
    if (v == nullptr) {
        printf("[FENCE] ERROR: Null vertex pointer\r\n");
        return false;
    }
    
    // Limpiar vértices anteriores
    clearVertices();
    
    // Copiar vértices al array estático
    memcpy(vertices, v, count * sizeof(Vertex));
    vertexCount = count;
    
    printf("[FENCE] Saved %d vertices successfully\r\n", vertexCount);
    return true;
}

void Fence::createLimits() {
    limitCount = 0;
    
    // Validar que haya al menos 2 vértices para formar líneas
    if (vertexCount < 2) {
        printf("[FENCE] WARNING: Need at least 2 vertices to create limits (have %d)\r\n", vertexCount);
        return;
    }

    // Crear líneas conectando vértices consecutivos
    for (uint8_t i = 0; i < vertexCount; i++) {
        limites[i].start = vertices[i];
        limites[i].end = vertices[(i + 1) % vertexCount];  // Cierre del polígono
        limitCount++;
    }

    // Actualizar centro del cerco
    updateCenterFence();
    
    printf("[FENCE] Created %d limits from %d vertices\r\n", limitCount, vertexCount);
}

void Fence::clearVertices() {
    vertexCount = 0;
    limitCount = 0;
    centerFence = {0.0, 0.0};
    
    // Limpiar arrays (opcional, por seguridad)
    memset(vertices, 0, sizeof(vertices));
    memset(limites, 0, sizeof(limites));
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void Fence::updateCenterFence() {
    if (vertexCount == 0) {
        centerFence = {0.0, 0.0};
        return;
    }

    double latSum = 0.0;
    double lonSum = 0.0;

    // Calcular promedio de coordenadas
    for (uint8_t i = 0; i < vertexCount; i++) {
        latSum += vertices[i].latitude;
        lonSum += vertices[i].longitude;
    }

    centerFence.latitude = latSum / vertexCount;
    centerFence.longitude = lonSum / vertexCount;
    
    printf("[FENCE] Center updated: (%.6f, %.6f)\r\n", centerFence.latitude, centerFence.longitude);
}

// ============================================================================
// ZONE THRESHOLDS
// ============================================================================

void Fence::setZoneThresholds() {
    // Verificar que los umbrales estén en orden lógico (decreciente)
    if (!(lightBlue > blue && blue > darkBlue && darkBlue > yellow && yellow > red)) {
        printf("[FENCE] ERROR: Thresholds must be in descending order\r\n");
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
    
    printf("[FENCE] Thresholds set: LIGHT_BLUE=%.1fm, BLUE=%.1fm, DARK_BLUE=%.1fm, YELLOW=%.1fm, RED=%.1fm\r\n",
           lightBlue, blue, darkBlue, yellow, red);
}

float Fence::getThreshold(zone_t zone) const {
    // Convertir zone_t enum a índice de array (zone: 1-5 → index: 0-4)
    if (zone >= LIGHT_BLUE_ZONE && zone <= RED_ZONE) {
        return thresholds[zone - 1];  // zone 1 → index 0, zone 5 → index 4
    }
    
    printf("[FENCE] WARNING: Invalid zone %d\r\n", zone);
    return -1.0f;  // No aplica
}

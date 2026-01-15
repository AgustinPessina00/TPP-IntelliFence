#ifndef FENCE_H
#define FENCE_H

#include "zone.h"
#include <stdint.h>
#include <stdio.h>

// ============================================================================
// EMBEDDED-FRIENDLY FENCE IMPLEMENTATION (NO STL, STATIC ALLOCATION)
// ============================================================================

#define MAX_VERTICES 10      // Máximo número de vértices del cerco
#define TOTAL_TRESHOLDS 5    // Número de umbrales de zona

typedef float threshold_t;

struct Vertex {
    float latitude;
    float longitude;
};

struct Line {
    Vertex start;
    Vertex end;
};

class Fence {
public:
    Fence();

    // ===== LIMIT MANAGEMENT =====
    /**
     * @brief Crear segmentos (límites) directamente desde buffer de vértices externo
     * @param v Puntero al array de vértices (no se guarda internamente)
     * @param count Número de vértices (max MAX_VERTICES)
     * @note No guarda los vértices, solo genera los límites
     */
    void createLimits(const Vertex* v, uint8_t count);

    // ===== GETTERS =====
    const Line* getLimits() const { return limites; }
    uint8_t getLimitCount() const { return limitCount; }
    
    Vertex getCenterFence() const { return centerFence; }

    // ===== ZONE THRESHOLDS =====
    /**
     * @brief Configurar umbrales de zona desde el límite del polígono (en metros)
     * @note Por defecto: GREEN(0-10m), ORANGE(10-20m), RED(20-30m), BLACK(>30m)
     */
    void setZoneThresholds();
    
    /**
     * @brief Obtener umbral de una zona específica
     * @param zone Zona a consultar
     * @return Distancia en metros desde el límite
     */
    float getThreshold(zone_t zone) const;

    // Array público de umbrales (compatible con código legacy)
    float thresholds[TOTAL_TRESHOLDS];

private:
    void updateCenterFence(const Vertex* v, uint8_t count);

    // ===== STATIC ARRAYS (NO DYNAMIC ALLOCATION) =====
    Line limites[MAX_VERTICES];       // Array estático de límites
    uint8_t limitCount;               // Cantidad actual de límites

    Vertex centerFence;               // Centro promedio de los vértices

    // Umbrales desde el límite hasta la respectiva zona (en metros)
    threshold_t lightBlue;
    threshold_t blue;
    threshold_t darkBlue;
    threshold_t yellow;
    threshold_t red;
};

#endif
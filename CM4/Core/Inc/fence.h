#ifndef FENCE_H
#define FENCE_H

#include "zone.h"
#include <stdint.h>
#include <stdio.h>

// ============================================================================
// EMBEDDED-FRIENDLY FENCE IMPLEMENTATION (NO STL, STATIC ALLOCATION)
// ============================================================================

#define MAX_VERTICES 20      // Máximo número de vértices del cerco
#define TOTAL_TRESHOLDS 5    // Número de umbrales de zona

typedef float threshold_t;

struct Vertex {
    double latitude;
    double longitude;
};

struct Line {
    Vertex start;
    Vertex end;
};

class Fence {
public:
    Fence();

    // ===== VERTEX MANAGEMENT =====
    /**
     * @brief Guardar vértices del cerco recibidos de LoRa
     * @param v Puntero al array de vértices
     * @param count Número de vértices (max MAX_VERTICES)
     * @return true si se guardaron exitosamente, false si count > MAX_VERTICES
     */
    bool saveVertices(const Vertex* v, uint8_t count);
    
    /**
     * @brief Recalcular segmentos (límites) a partir de vértices
     * @note Debe llamarse después de saveVertices()
     */
    void createLimits();
    
    /**
     * @brief Limpiar vértices para actualización de cerco
     */
    void clearVertices();

    // ===== GETTERS =====
    const Vertex* getVertices() const { return vertices; }
    uint8_t getVertexCount() const { return vertexCount; }
    
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
    void updateCenterFence();

    // ===== STATIC ARRAYS (NO DYNAMIC ALLOCATION) =====
    Vertex vertices[MAX_VERTICES];   // Array estático de vértices
    uint8_t vertexCount;              // Cantidad actual de vértices

    Line limites[MAX_VERTICES];       // Array estático de límites (mismo tamaño que vértices)
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

#ifndef FENCE_H
#define FENCE_H

#include <vector>
#include "zone.h"
#include "stdio.h"

#define TOTAL_TRESHOLDS  5

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

    void addVertex(const Vertex& v);
    void updateLimits();    // recalcula segmentos a partir de vértices
    void clearVertex();     // Elimina los vertices para luego cargar los nuevos cuando actualizamos el cerco

    std::vector<Vertex>& getVertices() const;
    std::vector<Line>& getLimits() const;
    Vertex getCenterFence() const;

    // Setea umbrales para cada zona desde el límite del polígono
    void setZoneThresholds();

    // Devuelve los umbrales definidos
    float getThreshold(zone_t zone) const;

    float thresholds[TOTAL_TRESHOLDS];

private:
    void updateCenterFence();

    std::vector<Vertex> vertices;
    std::vector<Line> limites;

    Vertex centerFence;     // Centro promedio de los vertices.

    // Umbrales desde el límite hasta la respectiva zona (en metros)
    threshold_t lightBlue = 20.0f;
    threshold_t blue = 15.0f;
    threshold_t darkBlue = 10.0f;
    threshold_t yellow = 5.0f;
    threshold_t red = 1.0f;
};

#endif

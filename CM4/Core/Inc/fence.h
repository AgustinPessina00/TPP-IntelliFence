
#ifndef FENCE_H
#define FENCE_H

#include <vector>

typedef float threshold_t;

enum zone_t {
    GREEN_ZONE,
    BLUE_ZONE,
    YELLOW_ZONE,
    RED_ZONE,
    BLACK_ZONE
};

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

    void addVertex(const Vertex& v);
    void updateLimites();  // recalcula segmentos a partir de vértices

    const std::vector<Vertex>& getVertices() const;
    const std::vector<Line>& getlimites() const;

    // Setea umbrales para cada zona desde el límite del polígono
    void setZoneThresholds(threshold_t blue, threshold_t yellow, threshold_t red);

    // Devuelve los umbrales definidos
    float getThreshold(zone_t zone) const;

private:
    std::vector<Vertex> vertices;
    std::vector<Line> limites;

    float thresholds[BLACK_ZONE];

    // Umbrales desde el límite hasta la respectiva zona (en metros)
    threshold_t blue = 10.0f;
    threshold_t yellow = 5.0f;
    threshold_t red = 1.0f;
};

#endif

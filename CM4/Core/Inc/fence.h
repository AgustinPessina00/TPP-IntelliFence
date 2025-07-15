
#ifndef FENCE_H
#define FENCE_H

#include <vector>

typedef float threshold_t;

enum zone_t {
    GREEN_ZONE = 0,
    BLUE_ZONE = 1,
    YELLOW_ZONE = 2,
    RED_ZONE = 3,
    BLACK_ZONE = 4
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
    void updateLimits();  // recalcula segmentos a partir de vértices

    const std::vector<Vertex>& getVertices() const;
    const std::vector<Line>& getLimits() const;

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

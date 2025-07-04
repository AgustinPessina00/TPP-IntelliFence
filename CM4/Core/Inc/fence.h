
#ifndef FENCE_H
#define FENCE_H

#include <vector>

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
    void updateSegments();  // recalcula segmentos a partir de vértices

    const std::vector<Vertex>& getVertices() const;
    const std::vector<Line>& getSegments() const;

    // Setea umbrales para cada zona desde el límite del polígono
    void setZoneThresholds(float blue, float yellow, float red, float black);

    // Devuelve los umbrales definidos
    float getBlueThreshold() const;
    float getYellowThreshold() const;
    float getRedThreshold() const;
    float getBlackThreshold() const;

private:
    std::vector<Vertex> vertices;
    std::vector<Line> segments;

    // Umbrales (en metros)
    float blueThreshold = 5.0f;
    float yellowThreshold = 10.0f;
    float redThreshold = 20.0f;
    float blackThreshold = 25.0f;
};

#endif

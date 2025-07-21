
#ifndef FENCE_H
#define FENCE_H

#include <vector>

typedef float threshold_t;

typedef enum {
    GREEN_ZONE       = 0x00,  //  Dentro del cerco virtual (sin estímulo)
    
    LIGHT_BLUE_ZONE  = 0x01,  //  Buzzer leve (frecuencia baja, duty bajo)
    BLUE_ZONE        = 0x02,  //  Buzzer medio
    DARK_BLUE_ZONE   = 0x03,  //  Buzzer intenso
    
    YELLOW_ZONE      = 0x04,  //  Buzzer + vibración
    
    RED_ZONE         = 0x05,  //  Vibración intensa (sin shock)
    BLACK_ZONE       = 0x06   //  Se escapó → alerta crítica
} zone_t;


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

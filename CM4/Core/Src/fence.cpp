
#include "fence.h"

Fence::Fence() {}

void Fence::addVertex(const Vertex& v)
{
    vertices.push_back(v);
    updateLimites();  // Siempre actualizamos las líneas
}

void Fence::updateLimites()
{
    limites.clear();
    if (vertices.size() < 2) return; // Podemos agregar algún manejo de error.

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        Line lim;
        lim.start = vertices[i];
        lim.end = vertices[(i + 1) % vertices.size()];  // cierre del polígono
        limites.push_back(lim);
    }
}

const std::vector<Vertex>& Fence::getVertices() const
{
    return vertices;
}

const std::vector<Line>& Fence::getLimites() const
{
    return limites;
}

void Fence::setZoneThresholds(threshold_t blue, threshold_t yellow, threshold_t red)
{
    // Verificar que los umbrales estén en orden lógico
    if (!(blue < yellow && yellow < red)) {
        printf("Error: los umbrales deben ser crecientes\r\n");
        return;
    }

    thresholds[BLUE_ZONE] = blue;
    thresholds[YELLOW_ZONE] = yellow;
    thresholds[RED_ZONE] = red;

}

float Fence::getThreshold(zone_t zone) const
{
    if (zone >= BLUE_ZONE && zone <= RED_ZONE)
        return thresholds[zone];
    else
        return -1.0f;  // No aplica
}
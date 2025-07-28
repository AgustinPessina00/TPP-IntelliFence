
#include "fence.h"

Fence::Fence() {}

void Fence::addVertex(const Vertex& v)
{
    vertices.push_back(v);
    updateLimites();  // Siempre actualizamos las líneas
}

void Fence::updateLimits()
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

    updateCenterFence();
}

void Fence::clearVertex()
{
    vertices.clear();
    limites.clear();
    centerFence = {0.0, 0.0};
}

const std::vector<Vertex>& Fence::getVertices() const
{
    return vertices;
}

const std::vector<Line>& Fence::getLimits() const
{
    return limites;
}

const Vertex Fence::getCenterFence()
{
    return centerFence;
}

void Fence::updateCenterFence()
{
    if(vertices.empty())
        return;

    double latSum = 0.0;
    double lonSum = 0.0;

    for(size_t i = 0; i < vertices.size(); i++) {
        latSum += vertices[i].latitude
        lonSum += vertices[i].longitude;
    }

    center.latitude = latSum / vertices.size();
    center.longitude = lonSum / vertices.size();    
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
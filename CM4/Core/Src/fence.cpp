
#include "fence.h"

Fence::Fence() {}

void Fence::addVertex(const Vertex& v)
{
    vertices.push_back(v);
    updateLimits();  // Siempre actualizamos las líneas
}

void Fence::updateLimits()
{
    limites.clear();
    if (vertices.size() < 2) return; // Podemos agregar algún manejo de error.

    for (size_t i = 0; i < vertices.size(); ++i) {
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

std::vector<Vertex>& Fence::getVertices() const
{
    return vertices;
}

std::vector<Line>& Fence::getLimits() const
{
    return limites;
}

Vertex Fence::getCenterFence() const
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
        latSum += vertices[i].latitude;
        lonSum += vertices[i].longitude;
    }

    centerFence.latitude = latSum / vertices.size();
    centerFence.longitude = lonSum / vertices.size();
}

void Fence::setZoneThresholds()
{
    // Verificar que los umbrales estén en orden lógico
    if (!(lightBlue > blue && blue > darkBlue &&  darkBlue > yellow && yellow > red)) {
        printf("Error: los umbrales deben ser crecientes\r\n");
        return;
    }

    thresholds[LIGHT_BLUE_ZONE] = lightBlue;
    thresholds[BLUE_ZONE] = blue;
    thresholds[DARK_BLUE_ZONE] = darkBlue;
    thresholds[YELLOW_ZONE] = yellow;
    thresholds[RED_ZONE] = red;

}

float Fence::getThreshold(zone_t zone) const
{
    if (zone >= LIGHT_BLUE_ZONE && zone <= RED_ZONE)
        return thresholds[zone - 1];
    else
        return -1.0f;  // No aplica
}


#include "fence.h"

Fence::Fence() {
	this->lightBlue = 20.0f;
	this->blue = 15.0f;
	this->darkBlue = 10.0f;
	this->yellow = 5.0f;
	this->red = 1.0f;

	setZoneThresholds();

	this->centerFence = {0.0, 0.0};
}

void Fence::saveVertices(const std::vector<Vertex> v){
	/*for (size_t i = 0; i < v.size(); i++) {
		addVertex(v[i]);
	}*/
	clearVertex();
	vertices.reserve(vertices.size() + v.size());  // evita realocaciones
	vertices.insert(vertices.end(), v.begin(), v.end());
}

void Fence::createLimits()
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

const std::vector<Vertex>& Fence::getVertices() const
{
    return vertices;
}

const std::vector<Line>& Fence::getLimits() const
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

    this->thresholds[LIGHT_BLUE_ZONE] = this->lightBlue;
    this->thresholds[BLUE_ZONE] = this->blue;
    this->thresholds[DARK_BLUE_ZONE] = this->darkBlue;
    this->thresholds[YELLOW_ZONE] = this->yellow;
    this->thresholds[RED_ZONE] = this->red;

}

float Fence::getThreshold(zone_t zone) const
{
    if (zone >= LIGHT_BLUE_ZONE && zone <= RED_ZONE)
        return thresholds[zone - 1];
    else
        return -1.0f;  // No aplica
}

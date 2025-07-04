
#include "fence.h"

Fence::Fence() {}

void Fence::addVertex(const Vertex& v)
{
    vertices.push_back(v);
    updateSegments();  // Siempre actualizamos las líneas
}

void Fence::updateSegments()
{
    segments.clear();
    if (vertices.size() < 2) return;

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        Line seg;
        seg.start = vertices[i];
        seg.end = vertices[(i + 1) % vertices.size()];  // cierre del polígono
        segments.push_back(seg);
    }
}

const std::vector<Vertex>& Fence::getVertices() const
{
    return vertices;
}

const std::vector<Line>& Fence::getEdges() const
{
    return segments;
}
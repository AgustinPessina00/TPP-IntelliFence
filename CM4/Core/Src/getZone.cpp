#include "getZone.h"
#include "math.h"

zone_t getZoneFromDistance(const Cow *cow, const Fence *fence, float &minDistance) {

    Position pos = cow->getPosition();
    Vertex center = fence->getCenterFence();

    XY cowXY = latLonToXY(pos.latitude, pos.longitude, center.latitude, center.longitude);

    const Line* limites = fence->getLimits();
    uint8_t limitCount = fence->getLimitCount();

    minDistance = calculateMinDistanceToFence(cowXY, center, limites, limitCount);

    if(isPointInsideFence(cowXY, limites, limitCount, center)) {
        // thresholds array: [0]=LIGHT_BLUE, [1]=BLUE, [2]=DARK_BLUE, [3]=YELLOW, [4]=RED
        if (minDistance > fence->thresholds[0]) return GREEN_ZONE;
        else if (minDistance > fence->thresholds[1]) return LIGHT_BLUE_ZONE;
        else if (minDistance > fence->thresholds[2]) return BLUE_ZONE;
        else if (minDistance > fence->thresholds[3]) return DARK_BLUE_ZONE;
        else if (minDistance > fence->thresholds[4]) return YELLOW_ZONE;
        else return RED_ZONE;
    }
    else {
        minDistance = -minDistance;
        return BLACK_ZONE;
    }
}

float calculateMinDistanceToFence(const XY cowXY, const Vertex center, const Line* limites, uint8_t limitCount) {

	float minDist = 1e10f;  // Inicializa la distancia minima con un valor muy grande

    for (uint8_t i = 0; i < limitCount; ++i) {
        const Line& seg = limites[i];
        XY a = latLonToXY(seg.start.latitude, seg.start.longitude, center.latitude, center.longitude);
        XY b = latLonToXY(seg.end.latitude, seg.end.longitude, center.latitude, center.longitude);
        
        float dist = pointToSegmentDistance(cowXY, a, b);
        
        if (dist < minDist) {
            minDist = dist;
        }
    }

    return minDist;
}

    
bool isPointInsideFence(const XY& pointXY, const Line* limites, uint8_t limitCount, const Vertex& center) {
    bool inside = false;

    for (uint8_t i = 0; i < limitCount; i++) {
        const Line& seg = limites[i];

        // Convertir los dos vértices del segmento a XY
        XY vi = latLonToXY(seg.start.latitude, seg.start.longitude, center.latitude, center.longitude);
        XY vj = latLonToXY(seg.end.latitude, seg.end.longitude, center.latitude, center.longitude);

        // Verificar si el segmento [vj, vi] cruza una línea horizontal desde pointXY
        bool intersect = ((vi.y > pointXY.y) != (vj.y > pointXY.y)) &&
                         (pointXY.x < (vj.x - vi.x) * (pointXY.y - vi.y) / (vj.y - vi.y + 1e-12) + vi.x); // +1e-12 para evitar división por cero

        if (intersect)
            inside = !inside;
    }

    return inside;
}


float pointToSegmentDistance(const XY& p, const XY& a, const XY& b) {
    XY ab = {b.x - a.x, b.y - a.y};
    XY ap = {p.x - a.x, p.y - a.y};

    double ab2 = ab.x * ab.x + ab.y * ab.y;
    double ap_ab = ap.x * ab.x + ap.y * ab.y;
    double t = ap_ab / ab2;

    // Limitar t al rango [0, 1]
    if (t < 0.0) t = 0.0;
    else if (t > 1.0) t = 1.0;

    XY proj = {a.x + t * ab.x, a.y + t * ab.y};
    double dx = p.x - proj.x;
    double dy = p.y - proj.y;

    return static_cast<float>(sqrt(dx * dx + dy * dy));
}

XY latLonToXY(double lat, double lon, double lat0, double lon0) {
    const double R = 6371000.0;  // Radio medio de la Tierra en metros
    double lat_rad = lat * M_PI / 180.0;
    double lon_rad = lon * M_PI / 180.0;
    double lat0_rad = lat0 * M_PI / 180.0;
    double lon0_rad = lon0 * M_PI / 180.0;

    double x = (lon_rad - lon0_rad) * cos(lat0_rad) * R;
    double y = (lat_rad - lat0_rad) * R;

    return {x, y};
}
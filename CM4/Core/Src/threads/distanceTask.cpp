#include "distanceTask.h"


void distanceToLimitTask(void *argument) {
    distanceTaskParams *distanceParams = static_cast<distanceTaskParams *>(argument);

    while(1) {
        // TODO: implementar lógica de la tarea
        
        float minDistance = calculateMinDistanceToFence(distanceParams->cow, distanceParams->fence);

        Message* msgReceived = nullptr;

        if (osMessageQueueGet(distanceToLimitQueueHandle, &msgReceived, NULL, 0) == osOK) {     // TODO: ¿POR QUÉ FIGURA EN BLANCO?
            Message* msg = new Message(MSG_ID_DISTANCE_TO_FENCE, ModuleId_t::DISTANCE, ModuleId_t::FSM, sizeof(float));  //[latitud, longitud]
            
            std::memcpy(msg->payload, &minDistance, sizeof(float));
            
            osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);    // TODO: ¿POR QUÉ FIGURA EN BLANCO?

            delete msgReceived;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

distance_t calculateMinDistanceToFence(const Cow *cow, const Fence *fence) {
    Position pos = cow.getPosition();
    Vertex center = fence.getCenter();

    // Convertir posición del animal a coordenadas XY relativas al centro del cerco
    XY cowXY = latLonToXY(pos.latitude, pos.longitude, center.latitude, center.longitude);

    float minDist = std::numeric_limits<float>::max();  // Inicializa la distancia minima con el máximo valor posible.

    const std::vector<Line>& limites = fence.getLimits();

    for (size_t i = 0; i < limites.size(); ++i) {
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



//ACA HAGO getZone:

//OPCION 1:

float thresholds[] = {10.0f, 15.0f, 20.0f, 25.0f, 30.0f, 40.0f}; // en metros

zone_t getZoneFromDistance(float dist, const float thresholds[]) {
    if (dist < thresholds[0]) return GREEN_ZONE;          // Antes de LIGHT_BLUE_ZONE
    else if (dist < thresholds[1]) return LIGHT_BLUE_ZONE;
    else if (dist < thresholds[2]) return BLUE_ZONE;
    else if (dist < thresholds[3]) return DARK_BLUE_ZONE;
    else if (dist < thresholds[4]) return YELLOW_ZONE;
    else if (dist < thresholds[5]) return RED_ZONE;
    else return BLACK_ZONE;
}

struct ZoneThresholds {
    float lightBlue;
    float blue;
    float darkBlue;
    float yellow;
    float red;
};

zone_t getZoneFromDistance(float dist, const ZoneThresholds& t) {
    if (dist < t.lightBlue) return GREEN_ZONE;
    else if (dist < t.blue) return LIGHT_BLUE_ZONE;
    else if (dist < t.darkBlue) return BLUE_ZONE;
    else if (dist < t.yellow) return DARK_BLUE_ZONE;
    else if (dist < t.red) return YELLOW_ZONE;
    else return RED_ZONE;  // BLACK_ZONE solo si se escapa mucho
}
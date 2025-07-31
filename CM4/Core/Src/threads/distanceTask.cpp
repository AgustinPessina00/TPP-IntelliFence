#include "distanceTask.h"
#include "math.h"

extern distanceToLimitQueueHandle;
extern dispatcherQueueHandle;

// TODO: Cambiar nombres de la task a getZoneTask.

void distanceToLimitTask(void *argument) {
    distanceTaskParams *distanceParams = static_cast<distanceTaskParams *>(argument);

    while(1) {     
        float minDistance;   
        zone_t zone = getZoneFromDistance(distanceParams->cow, distanceParams->fence, minDistance);

        uint8_t zoneCode = static_cast<uint8_t>(zone);
        
        Message* msgReceived = nullptr;
        Message* msgToSend = nullptr;

        if (osMessageQueueGet(distanceToLimitQueueHandle, &msgReceived, NULL, 0) == osOK) {

            switch (msgReceived->id) {
            case MSG_ID_REQUEST_ZONE_TO_FENCE:
            //TODO: Ver si pasamos zone_T en el mensaje o casteamos a uint8_t, ya que payload recibe uint8_t.
                msgToSend = new Message(MSG_ID_SEND_ZONE_TO_FENCE, ModuleId_t::DISTANCE, ModuleId_t::FSM, sizeof(uint8_t));
                std::memcpy(msgToSend->payload, &zoneCode, sizeof(uint8_t));
                break;
            case MSG_ID_REQUEST_DISTANCE_TO_FENCE:
                msgToSend = new Message(MSG_ID_SEND_DISTANCE_TO_FENCE, ModuleId_t::DISTANCE, ModuleId_t::FSM, sizeof(float));
                std::memcpy(msgToSend->payload, &minDistance, sizeof(float));
                break;
            case MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE:
                msgToSend = new Message(MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE, ModuleId_t::DISTANCE, ModuleId_t::FSM, 
                sizeof(uint8_t) + sizeof(float)); // 5 bytes
                std::memcpy(msgToSend->payload, &zoneCode, sizeof(uint8_t));
                std::memcpy(msgToSend->payload + sizeof(uint8_t), &minDistance, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;

            default:
                break;
            }
            
            }
            if(msgToSend)
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
            delete msgReceived;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

zone_t getZoneFromDistance(const Cow *cow, const Fence *fence, float &minDistance) {

    Position pos = cow->getPosition();
    Vertex center = fence->getCenter();

    XY cowXY = latLonToXY(pos.latitude, pos.longitude, center.latitude, center.longitude);

    const std::vector<Line>& limites = fence->getLimits();

    minDistance = calculateMinDistanceToFence(cowXY, center, limites);

    if(isPointInsideFence(cowXY, limites, center)) {
        if (minDistance > thresholds[LIGHT_BLUE_ZONE]) return GREEN_ZONE; 
        else if (minDistance > thresholds[BLUE_ZONE]) return LIGHT_BLUE_ZONE;
        else if (minDistance > thresholds[DARK_BLUE_ZONE]) return BLUE_ZONE;
        else if (minDistance > thresholds[YELLOW_ZONE]) return DARK_BLUE_ZONE;
        else if (minDistance > thresholds[RED_ZONE]) return YELLOW_ZONE;
        else return RED_ZONE;
    }
    else {
        minDistance = -minDistance;
        return BLACK_ZONE;
    }
}

distance_t calculateMinDistanceToFence(const XY cowXY, const Vertex center, const std::vector<Line>& limites) {
    float minDist = std::numeric_limits<float>::max();  // Inicializa la distancia minima con el máximo valor posible.

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

    
bool isPointInsideFence(const XY& pointXY, const std::vector<Line>& limites, const Vertex& center) {
    bool inside = false;

    for (size_t i = 0; i < limites.size() ; i++) {
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

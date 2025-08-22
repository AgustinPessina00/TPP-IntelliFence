#ifndef DISTANCETASK_H
#define DISTANCETASK_H

#include "cow.h"
#include "fence.h"
#include "messages.h"
#include <limits>

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"


typedef struct {
  Cow *cow;
  Fence *fence;
}distanceTaskParams;

struct XY {
    double x;
    double y;
};

void distanceToLimitTask(void *argument);

zone_t getZoneFromDistance(const Cow *cow, const Fence *fence, float &minDistance);

float calculateMinDistanceToFence(const XY cowXY, const Vertex center, const std::vector<Line>& limites);

bool isPointInsideFence(const XY& pointXY, const std::vector<Line>& limites, const Vertex& center);

float pointToSegmentDistance(const XY& p, const XY& a, const XY& b);

XY latLonToXY(double lat, double lon, double lat0, double lon0);

#ifdef __cplusplus
}
#endif

#endif // DISTANCETASK_H

#ifndef DISTANCETASK_H
#define DISTANCETASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "cow.h"
#include "fence.h"
#include "messages.h"

typedef struct {
  Cow *cow;
  Fence *fence;
}distanceTaskParams;

struct XY {
    double x;
    double y;
};

void distanceToLimitTask(void *argument);

zone_t getZoneFromDistance(const Cow *cow, const Fence *fence);

distance_t calculateMinDistanceToFence(const XY cowXY, const Vertex center, const std::vector<Line>& limites);

float pointToSegmentDistance(const XY& p, const XY& a, const XY& b);

XY latLonToXY(double lat, double lon, double lat0, double lon0);

#ifdef __cplusplus
}
#endif

#endif // DISTANCETASK_H

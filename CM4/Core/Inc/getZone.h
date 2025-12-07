/**
 ******************************************************************************
 * @file           : getZone.h
 * @brief          : Zone calculation and distance utilities
 * @author         : TPP-IntelliFence Team
 * @date           : November 26, 2025
 ******************************************************************************
 */

#ifndef GETZONE_H
#define GETZONE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "cow.h"
#include "fence.h"
#include "zone.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief XY coordinates structure (local Cartesian)
 */
typedef struct {
    double x;  /**< X coordinate in meters */
    double y;  /**< Y coordinate in meters */
} XY;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Calculate zone from cow distance to fence
 * @param cow Pointer to Cow object
 * @param fence Pointer to Fence object
 * @param minDistance Reference to store minimum distance to fence (output)
 * @return zone_t Current zone based on distance
 */
zone_t getZoneFromDistance(const Cow *cow, const Fence *fence, float &minDistance);

/**
 * @brief Calculate minimum distance from point to fence
 * @param cowXY Cow position in XY coordinates
 * @param center Fence center vertex
 * @param limites Pointer to array of fence limit lines
 * @param limitCount Number of limits in the array
 * @return float Minimum distance in meters
 */
float calculateMinDistanceToFence(const XY cowXY, const Vertex center, const Line* limites, uint8_t limitCount);

/**
 * @brief Check if point is inside fence using ray casting algorithm
 * @param pointXY Point coordinates in XY
 * @param limites Pointer to array of fence limit lines
 * @param limitCount Number of limits in the array
 * @param center Fence center vertex
 * @return true if point is inside fence, false otherwise
 */
bool isPointInsideFence(const XY& pointXY, const Line* limites, uint8_t limitCount, const Vertex& center);

/**
 * @brief Calculate distance from point to line segment
 * @param p Point coordinates
 * @param a Segment start point
 * @param b Segment end point
 * @return float Distance in meters
 */
float pointToSegmentDistance(const XY& p, const XY& a, const XY& b);

/**
 * @brief Convert latitude/longitude to local XY coordinates
 * @param lat Latitude in degrees
 * @param lon Longitude in degrees
 * @param lat0 Reference latitude (origin) in degrees
 * @param lon0 Reference longitude (origin) in degrees
 * @return XY Local Cartesian coordinates in meters
 */
XY latLonToXY(double lat, double lon, double lat0, double lon0);

#ifdef __cplusplus
}
#endif

#endif /* GETZONE_H */

#ifndef COW_H
#define COW_H

#include <stdint.h>
#include <array>
#include "zone.h"

enum class CowState {
    SLEEP,
    GRAZING,
    MOVEMENT
};

struct Position {
    double latitude;
    double longitude;
};

struct Acceleration {
    float ax;
    float ay;
    float az;
};

class Cow {
public:
    Cow(uint32_t id);

    void updatePosition(Position pos);
    void updateAcceleration(Acceleration accel);
    void updateState(CowState state);
    void updateCurrentZone(zone_t zone);
    void updateDistanceToLimit(float distance);

    uint16_t getId() const;
    Position getPosition() const;
    Acceleration getAcceleration() const;
    CowState getState() const;
    zone_t getCurrentZone() const;
    float getDistanceToLimit() const;

private:
    uint32_t id;
    Position position;
    Acceleration acceleration;
    CowState state;
    zone_t currentZone;
    float distanceToLimit;
};

#endif

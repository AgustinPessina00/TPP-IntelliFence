#ifndef COW_H
#define COW_H

//#include <stdint.h>
#include <cstdint>
#include "zone.h"

struct DeviceUID {
    uint32_t w0;
    uint32_t w1;
    uint32_t w2;
};

enum class CowState {
    SLEEP,
    GRAZING,
    MOVEMENT,
};

struct Position {
    double latitude;
    double longitude;
};

struct Acceleration {
    double ax;
    double ay;
    double az;
};

class Cow {
public:
	Cow(DeviceUID id);

    void updatePosition(Position pos);
    void updateAcceleration(Acceleration accel);
    void updateState(CowState state);
    void updateCurrentZone(zone_t zone);
    void updateDistanceToLimit(double distance);

    DeviceUID getId() const;
    Position getPosition() const;
    Acceleration getAcceleration() const;
    CowState getState() const;
    zone_t getCurrentZone() const;
    double getDistanceToLimit() const;

private:
    DeviceUID id;
    Position position;
    Acceleration acceleration;
    CowState state;
    zone_t currentZone;
    double distanceToLimit;
};

#endif
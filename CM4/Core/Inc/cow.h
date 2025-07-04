#ifndef COW_H
#define COW_H

#include <stdint.h>
#include <array>

enum class CowState {
    SLEEP,
    GRAZING,
    MOVEMENT
};

struct Position {
    float latitude;
    float longitude;
};

struct Acceleration {
    float ax;
    float ay;
    float az;
};

class Cow {
public:
    Cow(uint16_t id);

    void updatePosition(Position pos);
    void updateAcceleration(Acceleration accel);
    void updateState(CowState state);

    uint16_t getId() const;
    Position getPosition() const;
    Acceleration getAcceleration() const;
    CowState getState() const;

private:
    uint16_t id;
    Position position;
    Acceleration acceleration;
    CowState state;
};

#endif
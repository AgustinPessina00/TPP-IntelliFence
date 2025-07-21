
#include "cow.h"

Cow::Cow(uint16_t id) : id(id), state(CowState::SLEEP) {}

void Cow::updatePosition(Position pos) {
    position = pos;
}

void Cow::updateAcceleration(Acceleration accel) {
    acceleration = accel;
}

void Cow::updateState(CowState s) {
    state = s;
}

uint16_t Cow::getId() const {
    return id;
}

Position Cow::getPosition() const {
    return position;
}

Acceleration Cow::getAcceleration() const {
    return acceleration;
}

CowState Cow::getState() const {
    return state;
}
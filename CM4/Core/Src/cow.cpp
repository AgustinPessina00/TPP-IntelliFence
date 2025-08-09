
#include "cow.h"

// TODO: Falta declarar en el constructor los otros parámetros de cow.
Cow::Cow(uint16_t id) : id(id), state(CowState::SLEEP) {}

Cow::Cow(uint16_t id) {
    this->id = id;
    this->position = {0, 0};
    this->acceleration = {0, 0, 0};
    this->state = CowState::SLEEP;
    this->currentZone = zone_t::BLACK_ZONE;
    distanceToLimit = -1;
}

void Cow::updatePosition(Position pos) {
    position = pos;
}

void Cow::updateAcceleration(Acceleration accel) {
    acceleration = accel;
}

void Cow::updateState(CowState s) {
    state = s;
}

void Cow::updateCurrentZone(zone_t zone) {
    currentZone = zone;
}

void Cow::updateDistanceToLimit(float distance) {
    distanceToLimit = distance;
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

zone_t Cow::getCurrentZone() const {
    return currentZone;
}

float Cow::getDistanceToLimit() const
{
    return distanceToLimit;
}

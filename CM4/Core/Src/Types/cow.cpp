#include "cow.h"

Cow::Cow() {
    this->id = {0, 0, 0};
    this->position = {0, 0};
    this->acceleration = {0, 0, 0};
    this->state = CowState::SLEEP;
    this->currentZone = zone_t::BLACK_ZONE;
    this->distanceToLimit = -1;
    this->isInitialized = false;
}

Cow::Cow(DeviceUID id) {
    this->id = id;
    this->position = {0, 0};
    this->acceleration = {0, 0, 0};
    this->state = CowState::SLEEP;
    this->currentZone = zone_t::BLACK_ZONE;
    this->distanceToLimit = -1;
    this->isInitialized = true;
}

void Cow::init(DeviceUID id) {
    this->id = id;
    this->position = {0, 0};
    this->acceleration = {0, 0, 0};
    this->state = CowState::SLEEP;
    this->currentZone = zone_t::BLACK_ZONE;
    this->distanceToLimit = -1;
    this->isInitialized = true;
}

void Cow::updatePosition(Position pos) {
    if (!isInitialized) return;
    position = pos;
}

void Cow::updateAcceleration(Acceleration accel) {
    if (!isInitialized) return;
    acceleration = accel;
}

void Cow::updateState(CowState s) {
    if (!isInitialized) return;
    state = s;
}

void Cow::updateCurrentZone(zone_t zone) {
    if (!isInitialized) return;
    currentZone = zone;
}

void Cow::updateDistanceToLimit(float distance) {
    if (!isInitialized) return;
    distanceToLimit = distance;
}

DeviceUID Cow::getId() const {
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

float Cow::getDistanceToLimit() const {
    return distanceToLimit;
}

bool Cow::getIsInitialized() const {
    return isInitialized;
}
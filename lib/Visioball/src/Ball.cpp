#include "Ball.h"

Ball::Ball(Motion &motion) : motion_(motion) {}

bool Ball::update(IActuator &actuator) { return actuator.update(); }

Motion &Ball::motion() { return motion_; }

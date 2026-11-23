#include "Ball.h"


Ball::Ball(Motion &motion, Light &light, Sound &sound) //ctor
    : motion_(motion), light_(light), sound_(sound) {}

bool Ball::begin() { return motion_.begin(); } //start de motion sensor

bool Ball::update() {
  if (!motion_.update()) {
    return false;
  }
  const bool lightUpdated = light_.update();
  const bool soundUpdated = sound_.update();
  return lightUpdated && soundUpdated;
}

Motion &Ball::motion() { return motion_; }

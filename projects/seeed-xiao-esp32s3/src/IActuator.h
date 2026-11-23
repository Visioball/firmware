#pragma once

class IActuator {
public:
    virtual ~IActuator() = default;
    virtual void update() = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual void setLevel(float level) = 0;
};

#ifndef TWIDDLE_H
#define TWIDDLE_H

#include "motor.h"
#include <PID_v1.h>
#include "encoders.h"

namespace Twiddle {
    void autoTune(double& rpm, double set, PID& pid, double& correction, void (Motor::*setSpeed)(int), Motor* motor, Encoders &encoder);
};


#endif
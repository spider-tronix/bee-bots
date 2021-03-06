#include "twiddle.h"
#include "motor.h"
#include "encoders.h"

#include <stdlib.h>
#include <Arduino.h>

#include "PID.h"

#define PID_TUNING_ACCURACY 0.005
#define PID_SETTING_CYCLES 50

void Twiddle::autoTune(double& rpm, double set, PID_& pid, double& correction, void (Motor::*setSpeed)(int), Motor* motor, Encoders &encoder) {
    
    float best_err = abs(set - rpm);
    float err;

    double dp[3] = {0.01,0.01,0.01};
    double kp = pid.GetKp();
    double kd = pid.GetKd();
    double ki = pid.GetKi();

    double* parameters[] = {&kp, &kd, &ki};

    double sum = (dp[0] + dp[1] + dp[2]);
    while (sum > PID_TUNING_ACCURACY) {
        for (int i = 0; i < 3; i++) {
            *(parameters[i]) += dp[i];
            pid.SetTunings(kp, kd, ki);
            for (int i = 0; i < PID_SETTING_CYCLES; ++i) {
                pid.Compute();
                (motor->*setSpeed)(correction);
                encoder.computeRPM();
                delay(10);
            }            
            err = abs(set - rpm);
            Serial.print(kp);
            Serial.print("\t");
            Serial.print(kd);
            Serial.print("\t");
            Serial.print(ki);
            Serial.print("\n");

            if (err < best_err) {
                best_err = err;
                dp[i] *= 1.1;
            }
            else {
                *(parameters[i]) += dp[i];
                pid.SetTunings(kp, kd, ki);
                for (int i = 0; i < PID_SETTING_CYCLES; ++i) {
                    pid.Compute();
                    (motor->*setSpeed)(correction);
                    encoder.computeRPM();
                    delay(10);
                }            
                err = abs(set - rpm);
                Serial.print(kp);
                Serial.print("\t");
                Serial.print(kd);
                Serial.print("\t");
                Serial.print(ki);
                Serial.print("\n");

                if (err < best_err) {
                    best_err = err;
                    dp[i] *= 1.1;
                }
                else {
                    *(parameters[i]) += dp[i];
                    Serial.print(kp);
                    Serial.print("\t");
                    Serial.print(kd);
                    Serial.print("\t");
                    Serial.print(ki);
                    Serial.print("\n");
                    pid.SetTunings(kp, kd, ki);
                    dp[i] *= 0.9;
                }
            }
        }
        sum = (dp[0] +dp[1] + dp[2]); 
        Serial.println(sum);
    }   
}
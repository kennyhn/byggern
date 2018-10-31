#ifndef ADC_H_
#define ADC_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef enum Joystick_direction{LEFT, RIGHT, UP, DOWN,NEUTRAL}joystick_direction;
typedef struct Joystick_perc_angle{
    int8_t X_value;
    int8_t Y_value;
}joystick_perc_angle;

typedef struct Joystick_raw_data{
    uint8_t X_value;
    uint8_t Y_value;
}joystick_raw_data;

void joystick_y_axis(volatile uint8_t *adc);

void joystick_x_axis(volatile uint8_t *adc);

void r_slider(volatile uint8_t *adc);

void l_slider(volatile uint8_t *adc);

void run_joystick();

joystick_perc_angle get_perc_angle(volatile uint8_t *adc);

joystick_direction check_joystick_direction(volatile uint8_t *adc);

#endif

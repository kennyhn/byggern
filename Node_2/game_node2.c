#include "game_node2.h"
#include "adc_node2.h"
#include "uart_node2.h"
#include "pwm_node2.h"
#include "can_node2.h"
#include <util/delay.h>



int8_t check_game_over(void){
  uint16_t data = adc_read();
  //Call adc_read twice if the data IR is blocked to avoid start up issues.
  if (data < 50){
    adc_read();
    return 1;
  }
  return 0;
}

void play_game(void){

  joystick_direction joystick_dir;
  joystick_raw_data joystick_data;
  slider_raw_data slider;
  receive_console_message(&joystick_data,&joystick_dir, &slider);
  pwm_driver(joystick_data.X_value);
  position_reference=slider.right_slider_value;
  solenoid_control(joystick_data.button_pressed);

  //Check for game_over and send an empty message to node 1 to end the game
  if(check_game_over()){
    can_message empty_message;
    empty_message.id = 1;
    empty_message.length = 0;
      
    if (can_allowed_to_send_flag){
      can_message_send(&empty_message);
      can_allowed_to_send_flag=0;
    }
  }
}

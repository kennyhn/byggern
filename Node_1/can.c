#include "can.h"
#include "adc.h"
#include "timer.h"
#include "mcp2515.h"


int can_loopback_init(){
  if (mcp2515_init()){ //Setup mcp while checking if it is set up right
    return 1; //If mcp2515 is not set up right
  }
  mcp2515_write(MCP_CANCTRL, MODE_LOOPBACK); //Changing CAN to loopback mode
  if (!(mcp2515_read(MCP_CANSTAT) & MODE_LOOPBACK)){
    printf("The CAN is NOT in loopback mode\n");
    return 1;
  }
  printf("The CAN is in loopback mode\n");

  //Enable interrupts must be cleared by MCU to reset interrupt condition
  //mcp2515_write(MCP_CANINTE, MCP_TX_INT); //Enable transmit interrupts pin
  mcp2515_write(MCP_CANINTE, MCP_RX_INT); //enable recieve interrupts

  mcp2515_write(MCP_TXB0CTRL,0); //Make channel 0-2 ready to transmit message, setting all the transmit message flags to 0
  mcp2515_write(MCP_TXB1CTRL,0);
  mcp2515_write(MCP_TXB2CTRL,0);
  return 0;
}

int can_normal_init(){
  if (mcp2515_init()) //Setup mcp while checking if it is set up right
    return 1; //If mcp2515 is not set up right
  mcp2515_write(MCP_CNF1, 0x03);
  mcp2515_write(MCP_CNF2, 0x9A);
  mcp2515_write(MCP_CNF3, 0x07);

  mcp2515_write(MCP_CANCTRL, MODE_NORMAL); //Changing CAN to normal mode
  if ((mcp2515_read(MCP_CANSTAT) & MODE_MASK) != MODE_NORMAL){
    printf("The CAN is NOT in normal mode\n");
    return 1;
  }
  printf("The CAN is in normal mode\n");

  //Enable interrupts must be cleared by MCU to reset interrupt condition
  mcp2515_write(MCP_CANINTE, MCP_RX_INT); //enable recieve interrupts

  mcp2515_write(MCP_TXB0CTRL,0); //Make channel 0-2 ready to transmit message, setting all the transmit message flags to 0
  mcp2515_write(MCP_TXB1CTRL,0);
  mcp2515_write(MCP_TXB2CTRL,0);
  return 0;
}

void can_message_send(can_message* message){
  //Splitting id into higher and lower MSBs/LSBs
  unsigned id_high = message->id & 0b11111111000;
  unsigned id_low = message->id & 0b00000000111;
  id_low = id_low << 5;
  id_high = id_high >> 3;
  mcp2515_write(MCP_TXB0SIDH, id_high); //writing to TXB0SIDH, setting higher part address
  mcp2515_write(MCP_TXB0SIDL, id_low); //writing to TXB0SIDL, setting lower part address

  //Setting the data length, 4 lowest is 1
  mcp2515_write(MCP_TXB0DLC, message->length);

  //Sending data
  for(uint8_t i = 0; i < message->length; i++){
    mcp2515_write(MCP_TXB0D0+i, message->data[i]);
  }
  //Initiate transmission
  mcp2515_request_to_send(0x01);
  int i=0;
  //Interrupt check
  while(!(mcp2515_read_status() & MCP_TX0IF));
}

can_message can_data_receive(void){
  can_message message;
  while(!(mcp2515_read_status() & MCP_RX0IF));
  if(mcp2515_read_status() & MCP_RX0IF){
    message.id = (mcp2515_read(MCP_RXB0SIDH) << 3 | mcp2515_read(MCP_RXB0SIDL) >> 5);
    message.length = (0x0F) & mcp2515_read(MCP_RXB0DLC);

    for(uint8_t i = 0; i < message.length; i++){
      message.data[i] = mcp2515_read(MCP_RXB0D0+i);
    }
    mcp2515_write(MCP_CANINTF, mcp2515_read_status() & 0xFE); //clear interrupt flag
    return message;
  }
  message.id = 0;
  return message;
}

void send_console_message(uint8_t K_p,uint8_t K_i){
  //send joystick direction
  volatile uint8_t* adc = (uint8_t*)0x1400;
  joystick_direction joystick_dir = check_joystick_direction(adc);
  /*using raw data for greater resolution*/
  joystick_raw_data joystick_data;
  slider_raw_data slider;
  joystick_data.X_value=joystick_x_axis(adc);
  slider.right_slider_value= r_slider(adc);
  joystick_data.button_pressed = (PINB & (1<<PB2));

  can_message msg;
  msg.id = 10;
  msg.length = 5;
  msg.data[0]=joystick_data.X_value;
  msg.data[1]=joystick_data.button_pressed;
  msg.data[2]=slider.right_slider_value;
  msg.data[3]=K_p;
  msg.data[4]=K_i;
  if (can_allowed_to_send_flag){
    can_message_send(&msg);
    can_allowed_to_send_flag=0;
  }
}

//Check if there is an interrupt in CAN-controller
uint8_t can_int_vect(){
  return mcp2515_read(MCP_CANINTF);
}

void can_receive_interrupt_init(void){
  //  Set pin to input
  DDRD &= ~(1<<PD2);
  // Disable global interrupts
  cli();
  //Interrupt on falling edge PD2
  MCUCR |= (1<<ISC01);
  MCUCR &= ~(1<<ISC00);
  //Enable interrupt on PD2
  GICR |= (1<<INT0);

  //Enable global interrupts
  sei();
  can_message_received = 0;
}

ISR(INT0_vect){ //can message received interrupt
  can_message_received = 1;
}

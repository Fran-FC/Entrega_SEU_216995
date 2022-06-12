#ifndef __AUXILIAR_FUNCS_H__
#define __AUXILIAR_FUNCS_H__

#include "main.h"
#include "utility.h"


#define BUZZ_TIMEOUT 10


void write_LEDs(char * led_values);

void display_led_sensor_value(uint8_t alarm_led, double sensor_percentaje);

uint8_t is_button_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

size_t send_fast(BUFF_ITEM_t * buffer, size_t buffer_ct);

size_t send_to_modem(BUFF_ITEM_t * buffer, size_t buffer_ct, int time_delay, char * buffer_rcv);

void update_sensor_mode(uint8_t * pull_up_flag, uint8_t * sensor_mode);

float blink_led(uint8_t alarm_led, uint32_t * blink_timer, uint8_t * blink_flag);

float update_alarm_threshold(uint8_t * alarm_led, uint32_t *blink_timer, uint8_t *blink_flag);

float * read_POT();

float * read_LDR();

float * read_NTC();

void buzz(int flag, uint32_t * buzz_timer);

void display_mode(uint8_t * display_mode_flag);

void change_mode(uint8_t * change_mode_flag);

void buzz_SOS(uint8_t * current_state, uint32_t * timer);

char * strstrn(char * pajar, char * aguja, size_t area);

void strstrrm( char str[], char * t );

#endif

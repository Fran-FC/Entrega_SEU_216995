#include "auxiliar_funcs.h"
#include <stm32f4xx_hal_dma.h>
//#include "jsmn.h"

uint16_t led_pins[8] = {
		Led1_Pin, Led2_Pin, Led3_Pin, Led4_Pin,
		Led5_Pin, Led6_Pin, Led7_Pin, Led8_Pin
};

GPIO_TypeDef * led_gpios[8] = {
		Led1_GPIO_Port, Led2_GPIO_Port, Led3_GPIO_Port, Led4_GPIO_Port,
		Led5_GPIO_Port, Led6_GPIO_Port, Led7_GPIO_Port, Led8_GPIO_Port
};

extern UART_HandleTypeDef huart1;
#define UART_ESP_AT_WIFI (&huart1)

#define buffer_DMA_size 2048
uint8_t buffer_DMA[buffer_DMA_size];

#define BLINK_FREQ 20

void write_LEDs(char * led_values)
{
	for(int i=0; i<8; i++)
	{
		if(led_values[i] == '1')
			HAL_GPIO_WritePin(led_gpios[i], led_pins[i], GPIO_PIN_SET);
		if(led_values[i] == '0')
			HAL_GPIO_WritePin(led_gpios[i], led_pins[i], GPIO_PIN_RESET);
	}
}

void display_led_sensor_value(uint8_t alarm_led, double sensor_percentaje)
{
	// convert from double 0-100 range to int 0-7 range (LEDS ON)
	double aux2 = ceil(sensor_percentaje * 0.07);
	int leds_on = (int)aux2;

	int i = 0;

	for(; i < leds_on; i++)
		if(i != alarm_led)
			HAL_GPIO_WritePin(led_gpios[i], led_pins[i], GPIO_PIN_SET);

	for(; i < 8; i++)
		if(i != alarm_led)
			HAL_GPIO_WritePin(led_gpios[i], led_pins[i], GPIO_PIN_RESET);
}

uint8_t is_button_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET)
		return 1;

	return 0;
}



size_t send_to_modem(BUFF_ITEM_t * buffer, size_t buffer_ct, int time_delay, char * buffer_rcv)
{
	uint32_t buffer_ct1;
	// prepare reception buffer from ESPç

	HAL_UART_Receive_DMA(UART_ESP_AT_WIFI, buffer_DMA,buffer_DMA_size);
	// send line (command) to ESP
	HAL_UART_Transmit(UART_ESP_AT_WIFI,buffer,buffer_ct,1000);
	// wait a bit time
	osDelay(time_delay);
	//stop reception probably all data are in dma buffer
	HAL_UART_DMAStop(UART_ESP_AT_WIFI);

	// send to console ESP answer.
	buffer_ct1=buffer_DMA_size-HAL_DMA_getcounter(UART_ESP_AT_WIFI);
	buffer_ct=0;


	if(buffer_rcv != NULL)
	{
		//	buffer_rcv = (char *)malloc((size_t)1024*sizeof(char))
		while(buffer_ct <= buffer_ct1)
		{
			*(buffer_rcv+buffer_ct) = (char)buffer_DMA[buffer_ct];
			buffer_ct++;
		}
	}

	osDelay(1);

	return buffer_ct1;
}

void update_sensor_mode(uint8_t * pull_up_flag, uint8_t * sensor_mode)
{
	if(is_button_pressed(LeftButton_GPIO_Port, LeftButton_Pin) && *pull_up_flag == 0)
		*pull_up_flag = 1;
	else if(!is_button_pressed(LeftButton_GPIO_Port, LeftButton_Pin) && *pull_up_flag == 1)
	{
		*pull_up_flag = 0;
		if (*sensor_mode == LDR)
			*sensor_mode = NTC;
		else
			*sensor_mode = LDR;
	}
}

float blink_led(uint8_t alarm_led, uint32_t * blink_timer, uint8_t * blink_flag)
{
	float period = 1000/BLINK_FREQ; // ms
	if((HAL_GetTick() - *blink_timer) >= period)
	{
		*blink_timer = HAL_GetTick();
		if(*blink_flag)
		{
			HAL_GPIO_WritePin(led_gpios[alarm_led], led_pins[alarm_led], GPIO_PIN_SET);
			*blink_flag = 0;
		} else
		{
			HAL_GPIO_WritePin(led_gpios[alarm_led], led_pins[alarm_led], GPIO_PIN_RESET);
			*blink_flag = 1;
		}
	}
}

float update_alarm_threshold(uint8_t * alarm_led, uint32_t * blink_timer, uint8_t * blink_flag)
{
	float * res = read_POT();
	float threshold = res[1];
	free(res);
	*alarm_led = (uint8_t)round(threshold * 0.07);

	blink_led(*alarm_led, blink_timer, blink_flag);

	return threshold;
}

float * read_POT()
{
	float out = (float)readAnalogChannel( ADC_CHANNEL_4);
	// min 0, max 4095
	float threshold = (out / 4095.0) * 100.0; // %

	float * res = (float*)malloc(2*sizeof(float));
	*res = out;
	*(res+1) = threshold;

	return res;
}

float * read_LDR()
{
	float valueAD = (float)(4095 - readAnalogChannel(ADC_CHANNEL_0));
	float percentaje = ((valueAD - 1095.0) / 3000.0) * 100.0;

	float * res = (float*)malloc(2*sizeof(float));
	*res = valueAD;
	*(res+1) = percentaje;

	return res;
}

float * read_NTC()
{
	int BETA = 3900,
		T25 = 298,
		R25 = 10000;

	int valueAD = readAnalogChannel(ADC_CHANNEL_1);
	float aux = BETA
		/ (log(
		(-10000.0 * 3.3 / (valueAD * 3.3 / 4095.9 - 3.3)
		- 10000.0) / R25) + BETA / T25) - 273.18;

	float TEMP_MAX = 45;
	float TEMP_MIN = 10;


	float percentaje = (aux-TEMP_MIN)*100/(TEMP_MAX-TEMP_MIN);

	float * res = (float*)malloc(2*sizeof(float));
	*res = aux;
	*(res+1) = percentaje;

	return res;
}

void buzz(int flag, uint32_t * buzz_timer)
{
	if (flag)
	{
		if(is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
			*buzz_timer = HAL_GetTick();

		uint32_t time_passed = (HAL_GetTick() - *buzz_timer) / 1000; // seconds;

		if(time_passed >= BUZZ_TIMEOUT)
		{
			HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
			*buzz_timer = 0;
		}
		else
			HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
	}
	else
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
}

void buzz_SOS(uint8_t * current_state, uint32_t * timer)
{
	uint8_t SOS_state[27] = {
		0,1,0,1,0,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,0,0
	};
	uint32_t timeout = 100;

	uint32_t time_passed = HAL_GetTick() - *timer;

	if(time_passed >= timeout)
	{
		if(SOS_state[*current_state] == 0)
			HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);

		*timer = HAL_GetTick();
		*current_state = (*current_state+1)%sizeof(SOS_state);
	}
}

char * strstrn(char * pajar, char * aguja, size_t area)
{
    char * res = NULL;
    int i = 0, j;
    size_t len = strlen(aguja);
    while(i <= area)
    {
        if(*(pajar+i) == *aguja)
        {
            res = pajar+i;
            for(j = 1; (j + i) < area; ++j)
                if(*(res+j) != *(aguja+j))
                    break;

            if(j == len)
                break;

            res = NULL;
            i += j;
            continue;
        }
        i++;
    }
    return res;
}

void strstrrm( char str[], char * t )
{
    int i,j,k;
    i = 0;

    size_t t_len = strlen(t);

    while(i<strlen(str))
    {
        for (k = 0; k <= t_len; k++)
            if (str[i]==t[k])
                break;

        if (str[i]==t[k])
        {
            for (j=i; j<strlen(str); j++)
                str[j]=str[j+1];
        } else i++;
    }
}


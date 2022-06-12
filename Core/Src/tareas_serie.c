/*
 * tareas_auxiliares.c
 *
 *  Created on: May 3, 2022
 *      Author: pperez
 */

#include "FreeRTOS.h"
#include <stdio.h>
#include "tareas_serie.h"
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include "utility.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>
#include "auxiliar_funcs.h"
#include <stdlib.h>
#include "cJSON.h"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_rx2;

extern BUFF_BUFFER_t * buff;
extern  BUFF_BUFFER_t * buff_rx;

#define buffer_SIZE     512
extern uint8_t buffer_DMA_1[buffer_SIZE];
extern uint8_t buffer_DMA_2[buffer_SIZE];

#define BROKER_NAME "pperez2.disca.upv.es"
#define BROKER_PORT "10000"

#define DEVICE_ID "SensorSEU_FFC00"
#define CLON_ID "SensorSEU_PPB00"

#define QUERY_JSON "{\"entities\": [{\"type\": \"Sensor\",\"isPattern\": \"false\",\"id\": \"%s\"}]}"
#define UPDATE_JSON "{\"contextElements\":[{\"type\":\"Sensor\",\"isPattern\":\"false\",\"id\":\"%s\",\"attributes\":[{\"name\":\"Alarma\",\"type\":\"boolean\",\"value\":\"%c\"},{\"name\":\"Alarma_src\",\"type\":\"string\",\"value\":\"%s\"},{\"name\":\"IntensidadLuz\",\"type\":\"floatArray\",\"value\":\"%f,%f,%f,%f\"},{\"name\":\"Temperatura\",\"type\":\"floatArray\",\"value\":\"%f,%f,%f,%f\"}]}],\"updateAction\":\"APPEND\"}"

const char REQUEST_SKEL[2048] =
		"POST /v1/%s HTTP/1.1\r\nHost: %s:%s\r\nAccept: "
		"application/json\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n";

const char mode_names[4][10] = {
		"local",
		"connected",
		"clon",
		"test"
};

enum fa_tags {
	max =0,
	min=1,
	current=2,
	threshold=3
};

struct GlobalStatus globals;


void serie_Init_FreeRTOS(void)
{

	globals.CURRENT_CIPSTATE = ap_disconnected;
	globals.CURRENT_MODE = local;
	globals.CURRENT_SENSOR = LDR;
	globals.buzz_flag = 0;
	globals.xSem  = xSemaphoreCreateMutex();

	BaseType_t res_task;

	printf (PASCU_PRJ " at "__TIME__);
	fflush(0);

	buff=bufferCreat(128);
	if (!buff) return;

	buff_rx=bufferCreat(512);
	if (!buff_rx) return;


	res_task=xTaskCreate(Task_WIFI,"WIFI",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea WIFI\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Monitor,"MONITOR",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea MONITOR\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Mode,"MODE",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea MODE\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Display,"DISPLAY",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea DISPLAY\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_DMA,"DMA",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea DMA\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Receive,"RECEIVE",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea RECEIVE\r\n");
			fflush(NULL);
			while(1);
	}
}

void Task_DMA( void *pvParameters )
{
	uint32_t it,num;
	HAL_StatusTypeDef res;
	uint32_t nbuff;

    hdma_usart2_rx2.Instance = DMA1_Stream7;
    hdma_usart2_rx2.Init.Channel = DMA_CHANNEL_6;
    hdma_usart2_rx2.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx2.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx2.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_rx2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;


    if (HAL_DMA_Init(&hdma_usart2_rx2) != HAL_OK)
    {
      Error_Handler();
    }

	nbuff=0;
	res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_1,buffer_SIZE); // Para arrancar

	it=0;
	while(1){

		switch (nbuff){
		case 0: 	num=hdma_usart2_rx.Instance->NDTR;
					if (num<buffer_SIZE){
						__disable_irq();
						res=HAL_UART_DMAStop_PAS(&huart2);
					   __HAL_LINKDMA(&huart2,hdmarx,hdma_usart2_rx2);
					   res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_2,buffer_SIZE);
					   __enable_irq();
					   nbuff=1;
					   num=hdma_usart2_rx.Instance->NDTR;
					   res=buff->puts(buff_rx,buffer_DMA_1,buffer_SIZE-num);
					}else
						;

					break;
		case 1:
	    			num=hdma_usart2_rx2.Instance->NDTR;
	    			if (num<buffer_SIZE){
	    				__disable_irq();
	    				res=HAL_UART_DMAStop_PAS(&huart2);
	    				__HAL_LINKDMA(&huart2,hdmarx,hdma_usart2_rx);
	    				res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_1,buffer_SIZE);
	    				__enable_irq();
	    				nbuff=0;
	    				num=hdma_usart2_rx2.Instance->NDTR;
	    				res=buff->puts(buff_rx,buffer_DMA_2,buffer_SIZE-num);
	    			}else
	    				;
	    			break;
		}

		it++;
		vTaskDelay(1/portTICK_RATE_MS );

	}
}

void Task_Display( void *pvParameters )
{
	uint32_t it;
	BUFF_ITEM_t car;
	HAL_StatusTypeDef res;

    it=0;
	while(1)
	{
		buff->get(buff,&car);
		res=HAL_UART_Transmit(& huart2,&car,1,100);
		it++;

	}
}

void Task_Receive( void *pvParameters ){
	uint32_t it;
    uint32_t res;
    BUFF_ITEM_t  car;
#define buffer_length	128
    BUFF_ITEM_t  buffer[buffer_length];
    char resp[buffer_length];
    int buffer_ct,buffer_ct1;
    int crln_detect;

	it=0;

	while(1){
		it++;

		crln_detect=0;
		buffer_ct=0;

		while(crln_detect<2){
	    	res=buff->get(buff_rx,&car); // blocked
	    	buffer[buffer_ct++]=car;
	    	if (buffer_ct>1){

	    		if ((buffer[buffer_ct-2]=='\r')&&(buffer[buffer_ct-1]=='\n')) // \r\n detection end of line
					crln_detect=2;
				else
					if ((buffer_ct)==buffer_length)  // line out of limits --> error
						crln_detect=3;
	    	}
		}
		while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
		size_t size = send_to_modem(buffer, buffer_ct, 2000, resp);
		xSemaphoreGive(globals.xSem);
		buff->puts(buff, resp, size);
	}
}

void Task_WIFI( void *pvParameters )
{
	char wifi_creds[3][40] = {
			// NO HACER ESTO EN CASA
			"routerSEU 00000000",
			"MiFibra-385A iD9acgeL",
			"AndroidFran 00000000"

	};
	uint8_t number_of_wifis = sizeof wifi_creds / sizeof *wifi_creds;

	BUFF_ITEM_t cad[100];
	char * buffer_rx = (char *)malloc((size_t)1024*sizeof(char));

	while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	sprintf((char*)cad,"AT+RST\r\n");
	send_to_modem(cad, strlen(cad), 1, NULL);
	sprintf((char*)cad,"AT+CWQAP\r\n");
	send_to_modem(cad, strlen(cad), 1, NULL);
	sprintf((char*)cad,"AT+CWMODE=1\r\n");
	send_to_modem(cad, strlen(cad), 1, NULL);
	xSemaphoreGive(globals.xSem);

	uint8_t w = 0;
	while(1)
	{
		vTaskDelay(500);
		while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
		sprintf((char*)cad,"AT+CIFSR\r\n");
		size_t rx_size = send_to_modem(cad, strlen(cad), 300, buffer_rx);
		xSemaphoreGive(globals.xSem);

		if(strstrn(buffer_rx, "busy", rx_size) != NULL)
		{
			while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
			sprintf((char*)cad,"AT+RST\r\n");
			send_to_modem(cad, strlen(cad), 1, NULL);
			xSemaphoreGive(globals.xSem);
			vTaskDelay(1000);
			continue;
		}

		if(strstrn(buffer_rx, "+CIFSR:STAIP,\"192.", rx_size) != NULL)
		{
			globals.CURRENT_CIPSTATE = ap_connected;
			continue;
		}
		globals.CURRENT_CIPSTATE = ap_disconnected;
		char * passwd = strstr(wifi_creds[w], " ");
		passwd++;
		size_t ssid_len = passwd - wifi_creds[w] - 1;
		char ssid[31];
		memset(ssid, '\0', sizeof(ssid));
		strncpy(ssid, wifi_creds[w], ssid_len);
		w = (w+1)%number_of_wifis;

		while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
		sprintf((char*)cad,"AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, passwd);
		rx_size = send_to_modem(cad, strlen(cad), 2000, buffer_rx); // 4th. connect to AP
		xSemaphoreGive(globals.xSem);

		vTaskDelay(500);
	}
}

void Task_Monitor( void *pvParameters )
{
	// task that periodically makes requests to the context broker
	while(1)
	{
		if(globals.CURRENT_CIPSTATE != ap_connected) continue;
		if (globals.CURRENT_MODE == connected)
		{

			update_context();
			continue;
		}
		else if (globals.CURRENT_MODE == clon)
		{
			char * response = (char*)malloc(2048*sizeof(char));
			globals.clon_status_code = request_clon_info(response);

			free(response);
			continue;
		}
	}
}

void Task_Mode( void *pvParameters )
{
	uint32_t blink_timer = 0;
	uint32_t buzz_timer = 0;

	uint8_t blink_flag = 0;
	uint8_t pull_up_flag = 0;
	uint8_t change_mode_flag = 0;
	uint8_t display_mode_flag = 0;

	uint8_t alarm_led = 3;

	uint8_t test_led1 = 3;
	uint8_t test_led2 = 3;

	float * sensor_output;

	char led_values[8];

	uint8_t SOS_state = 0;

	while(1)
	{
		display_mode(&display_mode_flag);

		change_mode(&change_mode_flag);

		if (globals.CURRENT_MODE == local || globals.CURRENT_MODE == connected)
		{
			if(globals.CURRENT_MODE == connected &&
					globals.CURRENT_CIPSTATE == ap_disconnected)
				continue;

			update_sensor_mode(&pull_up_flag, &globals.CURRENT_SENSOR);
			float alarm_threshold = update_alarm_threshold(&alarm_led, &blink_timer, &blink_flag);

			if (globals.CURRENT_SENSOR == NTC)
				sensor_output = read_NTC();
			else
				sensor_output = read_LDR();

			display_led_sensor_value(alarm_led, sensor_output[1]);

			if(sensor_output[1] >= alarm_threshold)
				globals.buzz_flag = 1;
			else globals.buzz_flag = 0;

			buzz(globals.buzz_flag, &buzz_timer);

			if(globals.CURRENT_MODE == connected)
			{
				if(globals.CURRENT_SENSOR == NTC)
				{
					if(globals.temps[max] < sensor_output[0])
						globals.temps[max] = sensor_output[0];
					if(globals.temps[min] > sensor_output[0])
						globals.temps[min] = sensor_output[0];
					globals.temps[current] = sensor_output[0];
					globals.temps[threshold] = alarm_threshold;
				} else
				{
					if(globals.lums[max] < sensor_output[0])
						globals.lums[max]  = sensor_output[0];
					if(globals.lums[min]  > sensor_output[0])
						globals.lums[min]  = sensor_output[0];
					globals.lums[current]  = sensor_output[0];
					globals.lums[threshold]  = alarm_threshold;
				}
			}

			free(sensor_output);
		}
		if (globals.CURRENT_MODE == clon)
		{
			if(globals.CURRENT_CIPSTATE == ap_disconnected)
				continue;

			update_sensor_mode(&pull_up_flag, &globals.CURRENT_SENSOR);

			if(globals.clon_status_code < 0)
				continue;

			float percentaje_threshold;
			if (globals.CURRENT_SENSOR == NTC)
			{
				display_led_sensor_value(alarm_led, globals.temps[current]);
				percentaje_threshold = globals.temps[threshold];
			}
			else
			{
				percentaje_threshold = ((globals.lums[threshold] - 1095.0) / 3000.0) * 100.0;
				float percentaje_LDR =  ((globals.lums[current] - 1095.0) / 3000.0) * 100.0;
				display_led_sensor_value(alarm_led, percentaje_LDR);
			}

			alarm_led = (uint8_t)round(percentaje_threshold * 0.07);
			blink_led(alarm_led, &blink_timer, &blink_flag);

			if(is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
			{
				globals.buzz_flag = 0;
				globals.clon_status_code = 0; // 0 means TURN OFF CLON - CONNECTED ALARM
			}
			buzz(globals.buzz_flag, &buzz_timer);

			printf("----------------     \r\n"
					"temp: %f,%f,%f,%f   \r\n"
					"lum: %f,%f,%f,%f    \r\n"
					"alarm: %d           \r\n"
					"AT RESP: %s\r\n"
					"----------------    \r\n\033[6A",
					globals.temps[0],globals.temps[1],globals.temps[2],globals.temps[3],
					globals.lums[0],globals.lums[1],globals.lums[2],globals.lums[3],
					globals.buzz_flag);

			continue;
		}

		if (globals.CURRENT_MODE == test)
		{
			test_led1 = (test_led1 - 1) % 8;
			test_led2 = (test_led2 + 1) % 8;
			for(int i = 0; i < 8; i++)
			{
				if(i == test_led1 ||
						i == test_led2)
					led_values[i] = '1';
				else
					led_values[i] = '0';
			}
			write_LEDs(led_values);

			buzz_SOS(&SOS_state, &buzz_timer);

			uint8_t left_button_state = is_button_pressed(LeftButton_GPIO_Port, LeftButton_Pin);
			uint8_t right_button_state = is_button_pressed(RightButton_GPIO_Port, RightButton_Pin);

			float * LDR_output = read_LDR();
			float * NTC_output = read_NTC();
			float * POT_output = read_POT();

			char * buffer_rcv = (char*)malloc(20*sizeof(char));
			while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
			size_t recv_s = send_to_modem("AT\r\n", 4, 100, buffer_rcv);
			xSemaphoreGive(globals.xSem);
			printf("----------------        \r\n"
					"NTC: %.2fºC           \r\n"
					"LDR: %.2f%c           \r\n"
					"POT: %.0fV           \r\n"
					"Left button: %d    \r\n"
					"Right button: %d   \r\n"
					"AT RESP: %s         \r\n"
					"----------------     \r\n"
//					"----------------\r\n\033[7A"
					,
					NTC_output[0], LDR_output[1], '%',POT_output[0],
					left_button_state, right_button_state, buffer_rcv);

			vTaskDelay(50);

			free(buffer_rcv);
			free(LDR_output);
			free(NTC_output);
			continue;
		}

	}
}


/* AUXILIAR FUNCTIONS */

int request_clon_info(char * resp)
{
	char cad[100];
	char request_str[300];
	char request_body[300];

	sprintf(request_body, QUERY_JSON, CLON_ID);
	sprintf(request_str, REQUEST_SKEL, "updateContext", BROKER_NAME, BROKER_PORT, strlen(request_body), request_body);

	while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );

	sprintf(cad, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", BROKER_NAME, BROKER_PORT);
	send_to_modem(cad, strlen(cad),100, NULL);

	sprintf(cad,"AT+CIPSEND=%d\r\n", strlen(request_str));
	send_to_modem(cad, strlen(cad), 50, NULL);

	size_t resp_size = send_to_modem(request_str, strlen(request_str), 200, resp);

	sprintf(cad,"AT+CIPCLOSE\r\n");
	send_to_modem(cad, strlen(cad), 1, NULL);
	xSemaphoreGive(globals.xSem);

	if(resp_size < 900) return -1;

	int code = process_response(resp, resp_size);

	if(code < 0)
		return -1;

	return 1;
}

void update_context()
{
	char ALARM_STATE = globals.buzz_flag ? 'T' : 'F';
	char resp[2048];
	char cad[100];
	char request_str[612];
	char request_body[512];

	sprintf(request_body, "{\"contextElements\":[{\"type\":\"Sensor\",\"isPattern\":\"false\",\"id\":\"%s\""
			",\"attributes\":[{\"name\":\"Alarma\",\"type\":\"boolean\",\"value\":\"%c\"},{\"name\":\"Alarma_src\","
			"\"type\":\"string\",\"value\":\"%s\"},{\"name\":\"IntensidadLuz\",\"type\":\"floatArray\",\"value\":\"%f,%f,%f,%f\"},"
			"{\"name\":\"Temperatura\",\"type\":\"floatArray\",\"value\":\"%f,%f,%f,%f\"}]}],\"updateAction\":\"APPEND\"}",
			DEVICE_ID, ALARM_STATE, globals.alarm_src,
			globals.lums[0],globals.lums[1],globals.lums[2],globals.lums[3],
			globals.temps[0],globals.temps[1],globals.temps[2],globals.temps[3]);

	sprintf(request_str, REQUEST_SKEL, "updateContext", BROKER_NAME, BROKER_PORT, strlen(request_body), request_body);

	while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	sprintf(cad, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", BROKER_NAME, BROKER_PORT);
	send_to_modem(cad, strlen(cad),100, NULL);

	sprintf(cad,"AT+CIPSEND=%d\r\n", strlen(request_str));
	send_to_modem(cad, strlen(cad), 50, NULL);

	size_t resp_size = send_to_modem(request_str, strlen(request_str), 200, resp);

	sprintf(cad,"AT+CIPCLOSE\r\n");
	send_to_modem(cad, strlen(cad), 1, NULL);
	xSemaphoreGive(globals.xSem);


}

void display_mode(uint8_t * display_mode_flag)
{
	if(!(*display_mode_flag))
	{
		if(is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
		{
			*display_mode_flag = 1;
			if(globals.CURRENT_MODE == test)
				return;
			printf("CURRENT MODE: %s\r\n", mode_names[globals.CURRENT_MODE]);
		}
	}
	else if(!is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
		*display_mode_flag = 0;
}

void change_mode(uint8_t * change_mode_flag)
{
	if(!(*change_mode_flag))
	{
		if(is_button_pressed(LeftButton_GPIO_Port, LeftButton_Pin) &&
			is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
		{
			globals.CURRENT_MODE = (globals.CURRENT_MODE+1) % 4;
			*change_mode_flag = 1;
		}
	}
	else if(!is_button_pressed(LeftButton_GPIO_Port, LeftButton_Pin) &&
			!is_button_pressed(RightButton_GPIO_Port, RightButton_Pin))
		*change_mode_flag = 0;
}

int process_response(char * resp, size_t resp_size)
{
	int i, err_code = -1;
	if(resp == NULL)
		return err_code;
	for (i = 0; i <= resp_size; i++)
	{
		if(resp[i] == '{')
		{
			resp+=i;
			break;
		}
	}
	cJSON *attr = NULL,
			*contextResponse = NULL,
			*json = NULL;
	cJSON_Minify(resp);
	json = cJSON_Parse(resp);

	if(json == NULL)
		return err_code;

	cJSON *contextResponses = cJSON_GetObjectItemCaseSensitive(json, "contextResponses");
	cJSON_ArrayForEach(contextResponse, contextResponses)
	{
		cJSON *contextElement = cJSON_GetObjectItemCaseSensitive(contextResponse, "contextElement");
		cJSON *attributes = cJSON_GetObjectItemCaseSensitive(contextElement, "attributes");

		if(attributes == NULL)
			return err_code;

		char * c_value, *c_name;
		cJSON_ArrayForEach(attr, attributes)
		{
			cJSON *name = cJSON_GetObjectItemCaseSensitive(attr, "name");
			c_name = name->valuestring;
			cJSON *value = cJSON_GetObjectItemCaseSensitive(attr, "value");
			c_value = value->valuestring;

			err_code = i;
			i = 0;
			if(strcmp(c_name, "Alarma") == 0)
			{
				if(strcmp(c_value, "F") == 0) globals.buzz_flag = 0;
				else globals.buzz_flag = 1;
			}
			else if(strcmp(c_name, "Alarma_src") == 0)
			{
				if(globals.clon_status_code == 0)
				{
					char alarm_src[100];
					sprintf(alarm_src, "%s_%d",DEVICE_ID, rand());
					cJSON_SetValuestring(value,  alarm_src);
				}
			}
			else if (strcmp(c_name, "IntensidadLuz") == 0)
			{
				char *ptr = strtok(c_value, ",");

				while(ptr != NULL)
				{
					globals.lums[i++] = atof(ptr);
					ptr = strtok(NULL, ",");
				}
			}
			else if (strcmp(c_name, "Temperatura") == 0)
			{
				char *ptr = strtok(c_value, ",");

				while(ptr != NULL)
				{
					globals.temps[i++] = atof(ptr);
					ptr = strtok(NULL, ",");
				}
			}
		}
	}

	if(globals.clon_status_code == 0 && err_code != -1)
	{
		char request_str[600];
		char cad[100];

		char * request_body = cJSON_PrintUnformatted(json);
		sprintf(request_str, REQUEST_SKEL, "updateContext", BROKER_NAME, BROKER_PORT, strlen(request_body), request_body);

		while (xSemaphoreTake(globals.xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
		sprintf(cad, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", BROKER_NAME, BROKER_PORT);
		send_to_modem(cad, strlen(cad),100, resp);

		sprintf(cad,"AT+CIPSEND=%d\r\n", strlen(request_str));
		send_to_modem(cad, strlen(cad), 200, resp);

		size_t resp_size = send_to_modem(request_str, strlen(request_str), 800, resp);

		sprintf(cad,"AT+CIPCLOSE\r\n");
		send_to_modem(cad, strlen(cad), 1, NULL);
		xSemaphoreGive(globals.xSem);

		if(strstrn(resp, "ERROR", resp_size) == 0)
			printf("NO SE PUDO APAGAR LA ALARMA REMOTA\r\n");

		free(request_body);
	}

	free(contextResponses);
	free(attr);
	return err_code;
}


#ifndef __TAREAS_SERIE_H__
#define __TAREAS_SERIE_H__



void serie_Init_FreeRTOS(void);

void Task_Display( void *pvParameters );
void Task_DMA( void *pvParameters );
void Task_Receive( void *pvParameters );

void Task_WIFI( void *pvParameters );
void Task_Monitor( void *pvParameters );
void Task_Mode( void *pvParameters );

int request_clon_info(char * resp);
void update_context();
void display_mode(uint8_t * display_mode_flag);
void change_mode(uint8_t * change_mode_flag);
int process_response(char * resp, size_t resp_size);

#endif

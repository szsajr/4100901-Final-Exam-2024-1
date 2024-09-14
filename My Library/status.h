/*
 * status.h
 *
 *  Created on: Sep 13, 2024
 *      Author: UNALMAN
 */

#ifndef STATUS_H_
#define STATUS_H_

#include "stm32l4xx_hal.h"
// estados del sistema de seguridad
typedef enum {
	LED_OFF, // led_apagado
	LED_ON, // led encendido
	LED_BLINK, //led parpadenado
	LED_ERROR  // estado de error
} LED_State;

// prototipos de funciones

void LED_Init(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin); // Configuration del GPIO  LED
void LED_SetsState(LED_State state); //cambia el estado del led dependiendo del estado del sistema de seguridad
									// (encendido, apagado, parapadeando)
void LED_Toggle(void); //invierte el estado actual del led(útil para parpadeo)
#endif /* STATUS_H_ */

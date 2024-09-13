
#include "status.h"
static GPIO_TypeDef* led_GPIO_Port;
static uint16_t led_GPIO_Pin;
static LED_State current_state = LED_OFF;

//INICIALIZACION DEL LED, SPECIFICANDO EL PUERTO Y EL PIN
void LED_Init(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin){
	led_GPIO_Port = GPIO_Port;
	led_GPIO_Pin = GPIO_Pin;

// Configuración del GPIO para el LED
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIO_Port, &GPIO_InitStruct);

// Asegurarse que el LED esté apagado al inicio
	HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET);

}

void LED_SetState(LED_State state) {
    current_state = state;

    switch (state) {
        case LED_ON:
            HAL_GPIO_WritePin(led_GPIO_Port, led_GPIO_Pin, GPIO_PIN_SET);
            break;
        case LED_OFF:
            HAL_GPIO_WritePin(led_GPIO_Port, led_GPIO_Pin, GPIO_PIN_RESET);
            break;
        case LED_BLINK:
            // Parpadeo sencillo
            HAL_GPIO_TogglePin(led_GPIO_Port, led_GPIO_Pin);
            HAL_Delay(500); // Ajusta el tiempo según sea necesario
            break;
        case LED_ERROR:
            // Parpadeo rápido
            for (int i = 0; i < 5; i++) {
                HAL_GPIO_TogglePin(led_GPIO_Port, led_GPIO_Pin);
                HAL_Delay(200);
            }
            break;
    }
}

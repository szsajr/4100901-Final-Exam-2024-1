/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "ring_buffer.h"
#include "keypad.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

#define KEYPAD_RB_LEN 4
#define USART2_RB_LEN 6

uint8_t keypad_data = 0xFF;
uint8_t usart2_data = 0xFF;

uint8_t keypad_buffer[KEYPAD_RB_LEN];
ring_buffer_t keypad_rb;

uint8_t usart2_buffer[USART2_RB_LEN];
ring_buffer_t usart2_rb;

uint16_t keypad_input_value = 0;
uint32_t usart_input_value = 0;
uint32_t sum_result = 0;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

void process_sum(void);
void reset_values(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * Function to redirect printf output to UART.
 *
 * Purpose:
 * This function overrides the standard `_write` function to allow the
 * usage of `printf` in embedded systems without a standard output. It
 * redirects the printf output to UART for debugging or serial communication.
 *
 * Parameters:
 * - file: Not used, required by the function signature.
 * - ptr: Pointer to the data to be transmitted.
 * - len: Length of the data to be transmitted.
 *
 * Functionality:
 * - It sends the data pointed to by `ptr` via UART using HAL_UART_Transmit.
 * - It returns the length of the data transmitted.
 *
 * Use case:
 * By using this function, any call to `printf` will output the message
 * to the UART interface, which can be monitored with a serial console.
 */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 10);
  return len;
}
/*
 * Función para reiniciar los valores ingresados
 *
 * Propósito:
 * Esta función reinicia los valores ingresados tanto por el teclado
 * como por USART, limpiando los valores almacenados en las variables
 * `keypad_input_value` y `usart_input_value`.
 *
 * Funcionalidad:
 * - Restablece a 0 los valores de las entradas del keypad y USART.
 * - Limpia la pantalla OLED y muestra un mensaje indicando que los valores han sido reiniciados.
 * - Envía un mensaje a través de UART informando que los valores han sido reiniciados.
 *
 * Uso:
 * Se utiliza para restablecer el sistema cuando se presiona la tecla '*' en el teclado.
 */
void reset_values(void)
{
    keypad_input_value = 0;
    usart_input_value = 0;

    // Limpiar la pantalla OLED y mostrar un mensaje
    ssd1306_Fill(Black);
    ssd1306_SetCursor(10, 20);
    ssd1306_WriteString("Valores reiniciados", Font_6x8, White);
    ssd1306_UpdateScreen();

    // Mostrar mensaje en la UART
    printf("Valores reiniciados\r\n");
}


/*
 * Función para procesar la suma de los valores ingresados
 *
 * Propósito:
 * Esta función toma los valores ingresados a través del keypad y USART,
 * los suma y muestra el resultado tanto en la pantalla OLED como a través
 * de UART. Además, enciende o apaga un LED dependiendo de si el resultado es par o impar.
 *
 * Funcionalidad:
 * - Suma los valores almacenados en `keypad_input_value` y `usart_input_value`.
 * - Muestra el resultado de la suma en la pantalla OLED y lo envía por UART.
 * - Verifica si el resultado es par o impar:
 *   - Si es par, enciende el LED.
 *   - Si es impar, apaga el LED.
 *
 * Uso:
 * Se activa cuando se presiona el botón B1 en el sistema.
 */
void process_sum(void)
{
    sum_result = keypad_input_value + usart_input_value;

    // Mostrar el resultado en la pantalla OLED y en la UART
    ssd1306_Fill(Black);
    ssd1306_SetCursor(10, 20);
    char result_str[20];
    sprintf(result_str, "Suma: %lu", sum_result);
    ssd1306_WriteString(result_str, Font_6x8, White);

    // Imprimir si es par o impar en el OLED
    ssd1306_SetCursor(10, 40);
    if (sum_result % 2 == 0) {
        ssd1306_WriteString("Resultado: Par", Font_6x8, White);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);  // Encender LED si es par
    } else {
        ssd1306_WriteString("Resultado: Impar", Font_6x8, White);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);  // Apagar LED si es impar
    }

    ssd1306_UpdateScreen();
    printf("Resultado de la suma: %lu\r\n", sum_result);
}


/*
 * Callback de recepción de datos por USART2
 *
 * Propósito:
 * Esta función se ejecuta cada vez que se recibe un dato a través de la UART2.
 * Almacena los dígitos recibidos en un buffer circular y procesa el valor cuando
 * se ha recibido un número completo de hasta 6 dígitos.
 *
 * Funcionalidad:
 * - Almacena los dígitos recibidos (si son numéricos) en el buffer circular.
 * - Cuando se han recibido 6 dígitos, los convierte a un valor numérico.
 * - Imprime el valor recibido en la terminal UART y lo muestra en la pantalla OLED.
 *
 * Uso:
 * Esta función se llama automáticamente cuando se recibe un nuevo dato en la UART
 * y está configurada para trabajar con interrupciones.
 */
/* USART2 Callback ------------------------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // Si recibe un dígito, almacena en el buffer
        if (usart2_data >= '0' && usart2_data <= '9') {
            ring_buffer_write(&usart2_rb, usart2_data);
        }

        // Verifica si el buffer está lleno (6 dígitos)
        if (ring_buffer_size(&usart2_rb) >= 6) {
            char usart_str[7];
            for (int i = 0; i < 6; i++) {
                ring_buffer_read(&usart2_rb, (uint8_t *)&usart_str[i]);
            }
            usart_str[6] = '\0';
            usart_input_value = strtol(usart_str, NULL, 10);
            printf("USART Input: %lu\r\n", usart_input_value);

            // Imprimir valor recibido por USART en OLED
            ssd1306_Fill(Black);
            ssd1306_SetCursor(10, 30);  // Ajustar posición en la pantalla
            char usart_oled_str[20];
            sprintf(usart_oled_str, "USART: %lu", usart_input_value);
            ssd1306_WriteString(usart_oled_str, Font_6x8, White);
            ssd1306_UpdateScreen();
        }

        HAL_UART_Receive_IT(&huart2, &usart2_data, 1);
    }
}

/*
/*
 * Callback de manejo de interrupciones externas (Keypad y Botón B1)
 *
 * Propósito:
 * Esta función se ejecuta cuando se detecta una interrupción en los pines configurados
 * para el keypad o el botón B1. Procesa los dígitos ingresados por el keypad y realiza
 * la suma de valores cuando se presiona el botón B1.
 *
 * Funcionalidad:
 * - Si el botón B1 es presionado, se realiza la suma de los valores ingresados.
 * - Si se presiona el '*' en el keypad, se reinician los valores ingresados.
 * - Si se ingresan 4 dígitos en el keypad, estos se convierten en un valor numérico y
 *   se muestran tanto en la pantalla OLED como en la terminal UART.
 *
 * Uso:
 * Esta función se llama automáticamente cuando se detecta una interrupción en el pin
 * asociado al keypad o al botón B1.
 */
/* Keypad and button Callback ------------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin) {
        // Procesar suma cuando se presiona el botón B1
        process_sum();
        return;
    }

    // Manejo del keypad
    uint8_t key_pressed = keypad_scan(GPIO_Pin);
    if (key_pressed != 0xFF) {
        if (key_pressed == '*') {
            // Reiniciar valores si se presiona '*'
            reset_values();
        } else {
            ring_buffer_write(&keypad_rb, key_pressed);

            // Verifica si el buffer del keypad está lleno (4 dígitos)
            if (ring_buffer_size(&keypad_rb) >= 4) {
                char keypad_str[5];
                for (int i = 0; i < 4; i++) {
                    ring_buffer_read(&keypad_rb, (uint8_t *)&keypad_str[i]);
                }
                keypad_str[4] = '\0';
                keypad_input_value = strtol(keypad_str, NULL, 16);
                printf("Keypad Input: %u\r\n", keypad_input_value);

                // Imprimir valor ingresado por keypad en OLED
                ssd1306_Fill(Black);
                ssd1306_SetCursor(10, 40);  // Ajustar posición en la pantalla
                char keypad_oled_str[20];
                sprintf(keypad_oled_str, "Keypad: %u", keypad_input_value);
                ssd1306_WriteString(keypad_oled_str, Font_6x8, White);
                ssd1306_UpdateScreen();
            }
        }
    }
}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */


  // initialize the two circular buffers
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_RB_LEN);
  ring_buffer_init(&usart2_rb, usart2_buffer, USART2_RB_LEN);
  // intializing oled screen
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(20, 20);
  ssd1306_WriteString("starting arithmetic software", Font_7x10, White);
  ssd1306_UpdateScreen();

  HAL_UART_Receive_IT(&huart2, &usart2_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Starting arithmetic software \r\n");
  while (1)
  {
    /* USER CODE E ND WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */
  /*
   * Function to clear a specific line on the OLED screen.
   *
   * Purpose:
   * This function is used to "delete" the contents of a specific line
   * by writing spaces over it. This is useful when you want to update
   * or remove text from a specific position on the OLED display.
   *
   * Parameters:
   * - y_position: The vertical position (Y coordinate) of the line to clear.
   *
   * Functionality:
   * - The function places the cursor at the start of the line.
   * - It writes a string of spaces to overwrite any existing content on that line.
   * - Finally, it updates the screen to reflect the changes.
   */
  void clear_line(uint8_t y_position) {
      // Place the cursor at the beginning of the line
      ssd1306_SetCursor(10, y_position);
      // Write a line of spaces to "delete" the line
      ssd1306_WriteString("                ", Font_6x8, Black);  // Adjust the number of spaces according to the screen size
      ssd1306_UpdateScreen();
  }
  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 256000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ROW_4_GPIO_Port, ROW_4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ROW_3_Pin|ROW_2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ROW_4_Pin */
  GPIO_InitStruct.Pin = ROW_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(ROW_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ROW_1_Pin */
  GPIO_InitStruct.Pin = ROW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(ROW_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_3_Pin ROW_2_Pin */
  GPIO_InitStruct.Pin = ROW_3_Pin|ROW_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : COL_4_Pin */
  GPIO_InitStruct.Pin = COLUMN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COL_3_Pin COL_1_Pin COL_2_Pin */
  GPIO_InitStruct.Pin = COLUMN_3_Pin|COLUMN_1_Pin|COLUMN_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

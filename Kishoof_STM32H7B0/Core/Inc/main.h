/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Mode_Pin GPIO_PIN_2
#define Mode_GPIO_Port GPIOE
#define Warp_Polarity_Btn_Pin GPIO_PIN_3
#define Warp_Polarity_Btn_GPIO_Port GPIOE
#define ChB_Mix_Pin GPIO_PIN_12
#define ChB_Mix_GPIO_Port GPIOE
#define ChB_RM_Pin GPIO_PIN_13
#define ChB_RM_GPIO_Port GPIOE
#define B_Octave_Btn_Pin GPIO_PIN_9
#define B_Octave_Btn_GPIO_Port GPIOD
#define Octave_Up_Pin GPIO_PIN_0
#define Octave_Up_GPIO_Port GPIOD
#define Octave_Down_Pin GPIO_PIN_1
#define Octave_Down_GPIO_Port GPIOD
#define ENCODER_BTN_Pin GPIO_PIN_7
#define ENCODER_BTN_GPIO_Port GPIOD
#define LCD_DC_Pin GPIO_PIN_6
#define LCD_DC_GPIO_Port GPIOB
#define LCD_RESET_Pin GPIO_PIN_7
#define LCD_RESET_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

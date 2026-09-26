#include "main.h"
TIM_HandleTypeDef htim4;
volatile uint32_t gElapsed;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);

int main(void)
{
  volatile uint32_t d_mm = 0U;
  uint32_t measurement_start;
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM4_Init();

  while (1)
  {

    if (!HAL_GPIO_ReadPin(BUTTON_1_GPIO_Port, BUTTON_1_Pin)) 
    {
      while (!HAL_GPIO_ReadPin(BUTTON_1_GPIO_Port, BUTTON_1_Pin)); // aguarda o botao ser pressionado
    }

    gElapsed = 0U;
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    HAL_TIM_IC_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_2);

    // O HC-SR04 requer um pulso de trigger de pelo menos 10 us.
    // TIM4 opera a 1 MHz (1 tick = 1 us), então usamos o próprio timer
    // para gerar a largura do pulso sem depender de HAL_Delay (milissegundos).
    HAL_GPIO_WritePin(HCSR04_TRIGGER_GPIO_Port, HCSR04_TRIGGER_Pin, GPIO_PIN_SET);
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    while (__HAL_TIM_GET_COUNTER(&htim4) < 10U)
    {
    }
    HAL_GPIO_WritePin(HCSR04_TRIGGER_GPIO_Port, HCSR04_TRIGGER_Pin, GPIO_PIN_RESET);

    measurement_start = HAL_GetTick();
    while ((gElapsed == 0U) && ((HAL_GetTick() - measurement_start) < 30U))
    {
    }

    if (gElapsed != 0U)
    {
      // gElapsed está em microssegundos. A distância é metade do percurso
      // do som: d_mm = tempo_us * 343000 mm/s / (2 * 1e6).
      d_mm = (gElapsed * 343U + 1000U) / 2000U;
    }
    else
    {
      // Timeout: encerra a captura para a próxima medição.
      d_mm = 0U;
      HAL_TIM_IC_Stop(&htim4, TIM_CHANNEL_1);
      HAL_TIM_IC_Stop_IT(&htim4, TIM_CHANNEL_2);
    }

    HAL_Delay(60);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

// inicializa o timer 4
static void MX_TIM4_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 96-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim4) != HAL_OK) // Inicia o modo captura para ler eventos externos
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET; 
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  //configurações de input
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING; // captura evento de subida de degrau
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI; // atribui ao canal 1 à subida de degrau
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING; // captura evento de descida de degrau 
  sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI; // captura intervalo de tempo relativo ao canal 1
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(KIT_LED_GPIO_Port, KIT_LED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED_1_Pin|LED_2_Pin|LED_3_Pin|LED_4_Pin|LED_5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(HCSR04_TRIGGER_GPIO_Port, HCSR04_TRIGGER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : KIT_LED_Pin */
  GPIO_InitStruct.Pin = KIT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(KIT_LED_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_1_Pin|LED_2_Pin|LED_3_Pin|LED_4_Pin|LED_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON_2_Pin BUTTON_1_Pin */
  GPIO_InitStruct.Pin = BUTTON_2_Pin|BUTTON_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : HCSR04_TRIGGER_Pin */
  GPIO_InitStruct.Pin = HCSR04_TRIGGER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HCSR04_TRIGGER_GPIO_Port, &GPIO_InitStruct);

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if(htim == &htim4)
  { 
    if (htim4.Channel == HAL_TIM_ACTIVE_CHANNEL_2) // verifica se o callback foi chamado pela descida do degrau
    {   
      gElapsed = htim4.Instance->CCR2 - htim4.Instance->CCR1;   // ccr1 = contador quando o degrau subiu | ccr2 = contador quando o degrau desceu
      HAL_TIM_IC_Stop(&htim4, TIM_CHANNEL_1);
      HAL_TIM_IC_Stop_IT(&htim4, TIM_CHANNEL_2);       
    }
  }
}

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

/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2020 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

#include "gpio.h"
#include "main.h"
#include "syslog.h"

/* USER CODE BEGIN 0 */
#define CH_1 0
#define CH_2 1
#define CH_3 2
#define CH_4 3

/* USER CODE END 0 */

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim8;

extern channel_context_t chx_info[4];
uint8_t Start_Calculate,Start_Count;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim6)
	{
		Start_Count++;

		if (is_chx_enable(CH1))
        {
            chx_info[CH_1].timer_cnt = TIM2->CNT;               //读取定时器2的计数器值

            if (chx_info[CH_1].timer_cnt > chx_info[CH_1].freq_max)	//定标执行过程  取葡萄糖最大浓度值
                chx_info[CH_1].freq_max = chx_info[CH_1].timer_cnt;

            if (Start_Count == 2) {
                chx_info[CH_1].freq_min = chx_info[CH_1].timer_cnt;
            }

        /*33是反应30秒，23是反应20秒*/
            if (Start_Count >= 23) {
                channel_timer_on_off(CH1, STOP);
                chx_info[CH_1].freq_diff = chx_info[CH_1].freq_max - chx_info[CH_1].freq_min;

            //LOG_I("ch1: freq_max:[%d]\tfreq_min:[%d]\tfreq_diff:[%d]",chx_info[CH_1].freq_max, chx_info[CH_1].freq_min, chx_info[CH_1].freq_diff);
            }

            TIM2->CNT=0;
		}

        if (is_chx_enable(CH2))
        {
            chx_info[CH_2].timer_cnt = TIM3->CNT;	//读取定时器2的计数器值

			if (chx_info[CH_2].timer_cnt > chx_info[CH_2].freq_max)	//定标执行过程  取葡萄糖最大浓度值
                chx_info[CH_2].freq_max = chx_info[CH_2].timer_cnt;

			if (Start_Count == 2) {
                chx_info[CH_2].freq_min = chx_info[CH_2].timer_cnt;
            }

			if (Start_Count >= 23) {
                /*获取通道二真实的最大频率值*/
                channel_timer_on_off(CH2, STOP);
                chx_info[CH_2].freq_max = chx_info[CH_2].freq_max * 24 + 10;
                chx_info[CH_2].freq_min = chx_info[CH_2].freq_min * 24 + 10;
                chx_info[CH_2].freq_diff = chx_info[CH_2].freq_max - chx_info[CH_2].freq_min;
                //LOG_I("ch2: freq_max:[%d]\tfreq_min:[%d]\tfreq_diff:[%d]", chx_info[CH_2].freq_max, chx_info[CH_2].freq_min, chx_info[CH_2].freq_diff);
			}

			TIM3->CNT=0;
		}

		if (is_chx_enable(CH3))
		{
            chx_info[CH_3].timer_cnt = TIM1->CNT;

			if (chx_info[CH_3].timer_cnt > chx_info[CH_3].freq_max)	//定标执行过程  取葡萄糖最大浓度值
                chx_info[CH_3].freq_max = chx_info[CH_3].timer_cnt;

			if (Start_Count == 2) {
                chx_info[CH_3].freq_min = chx_info[CH_3].timer_cnt;
            }

			if (Start_Count >= 23)
			{
                channel_timer_on_off(CH3, STOP);
                chx_info[CH_3].freq_max = chx_info[CH_3].freq_max * 24 + 10;
                chx_info[CH_3].freq_min = chx_info[CH_3].freq_min * 24 + 10;
                chx_info[CH_3].freq_diff = chx_info[CH_3].freq_max - chx_info[CH_3].freq_min;
                //LOG_I("ch3: freq_max:[%d]\tfreq_min:[%d]\tfreq_diff:[%d]", chx_info[CH_3].freq_max, chx_info[CH_3].freq_min, chx_info[CH_3].freq_diff);
			}

			TIM1->CNT=0;
		}

		if (is_chx_enable(CH4))
		{
            chx_info[CH_4].timer_cnt = TIM8->CNT;

            if (chx_info[CH_4].timer_cnt > chx_info[CH_4].freq_max)	//定标执行过程  取葡萄糖最大浓度值
                chx_info[CH_4].freq_max = chx_info[CH_4].timer_cnt;

			if (Start_Count == 2) {
                chx_info[CH_4].freq_min = chx_info[CH_4].timer_cnt;
            }

			if (Start_Count >= 23)
			{
                channel_timer_on_off(CH4, STOP);
                chx_info[CH_4].freq_max = chx_info[CH_4].freq_max * 24 + 10;
                chx_info[CH_4].freq_min = chx_info[CH_4].freq_min * 24 + 10;
                chx_info[CH_4].freq_diff = chx_info[CH_4].freq_max - chx_info[CH_4].freq_min;
                //LOG_I("ch4: freq_max:[%d]\tfreq_min:[%d]\tfreq_diff:[%d]", chx_info[CH_4].freq_max, chx_info[CH_4].freq_min, chx_info[CH_4].freq_diff);
			}

			TIM8->CNT=0;
		}

        /*after 23s*/
		if (Start_Count >= 23)
		{
			Start_Count = 0;
			Start_Calculate = 0x01;						//定标检测结束标志位
			HAL_TIM_Base_Stop_IT(&htim6); 				//检测完成，关闭定时器6
			TIM6->CNT = 0;								//清除定时器6的计数值
		}

        //LOG_I("ch_%d: Start_Count:[%d], timer_cnt:[%d], freq_min:[%d]", CH_1+1, Start_Count, chx_info[CH_1].timer_cnt, chx_info[CH_1].freq_min);
        //LOG_I("ch_%d: Start_Count:[%d], timer_cnt:[%d], freq_min:[%d]", CH_2+1, Start_Count, chx_info[CH_2].timer_cnt, chx_info[CH_2].freq_min);
	}
}

/* TIM1 init function */
void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 23;						//分频系数  最大可以统计1500.000HZ的频率
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xffff;						//重装载值
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
/* TIM2 init function */
void MX_TIM2_Init(void)									//通道一
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
/* TIM3 init function */
void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 23;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 0xffff;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
/* TIM6 init function */
/*1秒进一次中断*/
void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 9999;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
/* TIM8 init function */
void MX_TIM8_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 23;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 0xffff;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(tim_baseHandle->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspInit 0 */

  /* USER CODE END TIM1_MspInit 0 */
    /* TIM1 clock enable */
    __HAL_RCC_TIM1_CLK_ENABLE();
  
    /**TIM1 GPIO Configuration    
    PA12     ------> TIM1_ETR 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM1_MspInit 1 */

  /* USER CODE END TIM1_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspInit 0 */

  /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
  
    /**TIM2 GPIO Configuration    
    PA5     ------> TIM2_ETR 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM2 interrupt Init */
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  /* USER CODE BEGIN TIM2_MspInit 1 */

  /* USER CODE END TIM2_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();
  
    /**TIM3 GPIO Configuration    
    PD2     ------> TIM3_ETR 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM6)
  {
  /* USER CODE BEGIN TIM6_MspInit 0 */

  /* USER CODE END TIM6_MspInit 0 */
    /* TIM6 clock enable */
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* TIM6 interrupt Init */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* USER CODE BEGIN TIM6_MspInit 1 */

  /* USER CODE END TIM6_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM8)
  {
  /* USER CODE BEGIN TIM8_MspInit 0 */

  /* USER CODE END TIM8_MspInit 0 */
    /* TIM8 clock enable */
    __HAL_RCC_TIM8_CLK_ENABLE();
  
    /**TIM8 GPIO Configuration    
    PA0-WKUP     ------> TIM8_ETR 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM8_MspInit 1 */

  /* USER CODE END TIM8_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{
  if(tim_baseHandle->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspDeInit 0 */

  /* USER CODE END TIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM1_CLK_DISABLE();
  
    /**TIM1 GPIO Configuration    
    PA12     ------> TIM1_ETR 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);

  /* USER CODE BEGIN TIM1_MspDeInit 1 */

  /* USER CODE END TIM1_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspDeInit 0 */

  /* USER CODE END TIM2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
  
    /**TIM2 GPIO Configuration    
    PA5     ------> TIM2_ETR 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);

    /* TIM2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
  /* USER CODE BEGIN TIM2_MspDeInit 1 */

  /* USER CODE END TIM2_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
  
    /**TIM3 GPIO Configuration    
    PD2     ------> TIM3_ETR 
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM6)
  {
  /* USER CODE BEGIN TIM6_MspDeInit 0 */

  /* USER CODE END TIM6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM6_CLK_DISABLE();

    /* TIM6 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
  /* USER CODE BEGIN TIM6_MspDeInit 1 */

  /* USER CODE END TIM6_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM8)
  {
  /* USER CODE BEGIN TIM8_MspDeInit 0 */

  /* USER CODE END TIM8_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM8_CLK_DISABLE();
  
    /**TIM8 GPIO Configuration    
    PA0-WKUP     ------> TIM8_ETR 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);

  /* USER CODE BEGIN TIM8_MspDeInit 1 */

  /* USER CODE END TIM8_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */
void timer_init(void)
{
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM1_Init();
    MX_TIM8_Init();
    MX_TIM6_Init();
}

void channel_timer_on_off(channel_t chx, bool state)
{
    int i = 0;
    channel_t channel_index = 1;
    if (chx & CH1) {
        if (state) {
            HAL_TIM_Base_Start_IT(&htim2);
        } else {
            HAL_TIM_Base_Stop_IT(&htim2);
        }
    }
    if (chx & CH2) {
        if (state) {
            HAL_TIM_Base_Start_IT(&htim3);
        } else {
            HAL_TIM_Base_Stop_IT(&htim3);
        }
    }
    if (chx & CH3) {
        if (state) {
            HAL_TIM_Base_Start_IT(&htim1);
        } else {
            HAL_TIM_Base_Stop_IT(&htim1);
        }
    }
    if (chx & CH4) {
        if (state) {
            HAL_TIM_Base_Start_IT(&htim8);
        } else {
            HAL_TIM_Base_Stop_IT(&htim8);
        }
    }
}

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

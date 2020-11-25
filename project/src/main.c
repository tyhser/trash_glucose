
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include <stdio.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "syslog.h"

#define FMAX(x) (chx_info[(x)].freq_max)
#define FMIN(x) (chx_info[(x)].freq_min)
#define FDIFF(x) (chx_info[(x)].freq_diff)
#define CH_1 0
#define CH_2 1
#define CH_3 2
#define CH_4 3

channel_context_t chx_info[4];

void SystemClock_Config(void);

extern uint8_t Start_Calculate;

uint8_t RX_Buf[14], RX_Flag, Order_Type;
uint8_t Feedback[44];				//定标及测样反馈数组

uint8_t active_channel_mask;

uint32_t LED_Count;			//LED灯

uint8_t read_active_channel_mask(uint8_t channel_words[4]);
void channel_timer_on_off(channel_t chx, bool state);

void reset_all_channel_freq_max(void);
void reset_all_channel_freq_min(void);
void reset_all_channel_calibrate_count(void);
void reset_all_channel_calibrate(void);

bool is_all_channel_calibrate_finish(void);
bool is_calibrate_any_channel_unknow_error(void);
bool is_calibrate_any_channel_known_error(void);

void calibrate_data_process(channel_t chx);

static void fill_calibrate_feedback_chx(uint8_t *feedback);
static void fill_concentration_feedback(uint8_t *feedback);
static void update_concentration(uint8_t ch_x);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    timer_init();
    MX_USART2_UART_Init();									//初始化串口6
    MX_USART1_UART_Init();									//初始化串口6
    HAL_UART_Receive_IT(&huart2,RX_Buf,14);			   //开启串口6接收中断

    MX_IWDG_Init();

    LOG_I("Init complete");
    while(1)
    {
		/********** 串口数据接收 **********/
        if(RX_Flag==2)
		{
			RX_Flag=0;

			/********** 通道选择 开始 **********/
			if(RX_Buf[3]==0x37)
			{
                LOG_I("Channel select");
                active_channel_mask |= read_active_channel_mask(&RX_Buf[4]);

                reset_all_channel_calibrate_count();
                reset_all_channel_freq_max();

                channel_timer_on_off(CH1|CH2|CH3|CH4, STOP);

				HAL_UART_Transmit_DMA(&huart2,RX_Buf,14);					//已接收到数据 进行反馈
                LOG_I("Send RX buf back");
 			}
			/********** 通道选择 结束 **********/

			/********** 接收到定标、测样指令 开始 **********/
			else if( RX_Buf[3]==0x33 )												//定标指令51  0x33
			{
                LOG_I("start to 定标");
				Order_Type=1;															//定标标志位

                channel_timer_on_off(active_channel_mask, START);
                LOG_I("Start active channel timer");

                LOG_I("start timer 2 3 1 8");
				HAL_TIM_Base_Start_IT(&htim6); 									//开启定时器6 基础定时器

                LOG_I("start basic timer 6");
				HAL_UART_Transmit_DMA(&huart2,RX_Buf,14);						//通讯反馈
                LOG_I("Send RX buf back");
			}

			else if( RX_Buf[3]==0x34 )												//定标结果查询52  0x34
            {
				HAL_UART_Transmit_DMA(&huart2,Feedback,44);					//进行定标反馈和频率上传
                LOG_I("upload feedback");
            }

			else if( RX_Buf[3]==0x35 )												//测样指令53  0x35
			{
                LOG_I("start to 测样");
                reset_all_channel_calibrate();
				Order_Type=2;															//测样标志位

                channel_timer_on_off(active_channel_mask, START);

				HAL_TIM_Base_Start_IT(&htim6); 									//开启定时器6 基础定时器
                LOG_I("start basic timer 6");
				HAL_UART_Transmit_DMA(&huart2,RX_Buf,14);						//通讯反馈
                LOG_I("Send RX buf back");
			}

			else if( RX_Buf[3]==0x36 )												//测样结果查询54  0x36
            {
				HAL_UART_Transmit_DMA(&huart2,Feedback,44);			//进行测样反馈和频率上传
                LOG_I("upload feedback");
            }

			/********** 接收到定标、测样指令 结束 **********/

		}

		/************ 定标 开始 ************/
		if((Start_Calculate==0x01) && (Order_Type==1)) {
            LOG_I("定标数据处理...");

            __disable_irq();
			Start_Calculate=0;
            __enable_irq();
			Order_Type=0;

			if (active_channel_mask & CH1)
			{
                calibrate_data_process(CH1);
			}
			if (active_channel_mask & CH2)
			{
                calibrate_data_process(CH2);
			}
			if (active_channel_mask & CH3)
			{
                calibrate_data_process(CH3);
			}
			if (active_channel_mask & CH4)
			{
                calibrate_data_process(CH4);
			}
			/************************ 定标结果判断 **********************/
            if (is_all_channel_calibrate_finish())
			{
                reset_all_channel_calibrate();
                Feedback[2]=0x05;											//定标通过――5		//继续定标是――1
			}
			/************************* 此为预留功能，勿删 *************************/
#ifdef CALIBRATE_ERROR_ENABLE
			else if(is_calibrate_any_channel_unknow_error()) {
				Feedback[2]=0x01;															//定标通过――5		//继续定标是――1
            } else if (is_calibrate_any_channel_known_error()) {
				Feedback[2]=0x00;
				Feedback[3]=chx_info[CH_1].calibrate;
				Feedback[4]=chx_info[CH_2].calibrate;
				Feedback[5]=chx_info[CH_3].calibrate;
				Feedback[6]=chx_info[CH_4].calibrate;

                reset_all_channel_calibrate();
			}
#else
			else			//使用预留功能时，这两行注释掉WTF
				Feedback[2]=0x01;	//定标通过――5		//继续定标是――1
#endif

        Feedback[0]=0x0a;
        Feedback[1]=0x0c;

        Feedback[3]=0x00;
        Feedback[4]=0x00;
        Feedback[5]=0x00;
        Feedback[6]=0x00;

        Feedback[7]=0x05;

        fill_calibrate_feedback_chx(&Feedback[8]);

        reset_all_channel_freq_max();
		}
		/************ 定标 结束 ************/

		/************ 浓度检测 开始 ************/
        if ((Start_Calculate==0x01) && (Order_Type==2))						//检测数据处理
        {
            LOG_I("检测数据处理...");
            __disable_irq();
            Start_Calculate=0;
            __enable_irq();
            Order_Type=0;

            if(active_channel_mask & CH1)				//通道一开启
            {
                update_concentration(CH_1);
            }

            if(active_channel_mask & CH2)				//通道二开启
            {
                update_concentration(CH_2);
            }

            if(active_channel_mask & CH3)				//通道三开启
            {
                update_concentration(CH_3);
            }

            if(active_channel_mask & CH4)				//通道四开启
            {
                update_concentration(CH_4);
            }

            fill_concentration_feedback(&Feedback);
            fill_calibrate_feedback_chx(&Feedback[8]);

            reset_all_channel_freq_max();
		}

		/************ 浓度检测 结束 ************/


		/********* LED灯闪烁 **********/
        LED_Count++;
        if(LED_Count%4255999==0) {
            LED_Count=0;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
        }
        HAL_IWDG_Refresh(&hiwdg);			//IWDG看门狗喂狗
    }
}

static void update_concentration(uint8_t ch_x)
{
    chx_info[ch_x].freq_diff = chx_info[ch_x].freq_max - chx_info[ch_x].freq_min;
    chx_info[ch_x].concentration = ((float)chx_info[ch_x].freq_diff/chx_info[ch_x].std_value) * 1000;
}

static void fill_calibrate_feedback_chx(uint8_t *feedback)
{
    calibrate_data_section_t data[4];
    for (int i = 0; i < CH_CNT; i++) {
        data[i].freq_max = chx_info[i].freq_max;
        data[i].freq_min = chx_info[i].freq_min;
        data[i].freq_diff = chx_info[i].freq_diff;
    }
    memcpy(feedback, data, sizeof(calibrate_data_section_t) * 4);
}

static void fill_concentration_feedback(uint8_t *feedback)
{
    concentration_data_t data;
    for (int i = 0; i < CH_CNT; i++) {
        data[i] = chx_info[i].concentration;
    }
    memcpy(feedback, data, sizeof(concentration_data_t));
}

void reset_all_channel_calibrate_count(void)
{
    chx_info[0].calibrate_cnt = 0;
    chx_info[1].calibrate_cnt = 0;
    chx_info[2].calibrate_cnt = 0;
    chx_info[3].calibrate_cnt = 0;
}

void reset_all_channel_freq_max(void)
{
     __disable_irq();
    chx_info[0].freq_max = 0;
    chx_info[1].freq_max = 0;
    chx_info[2].freq_max = 0;
    chx_info[3].freq_max = 0;
    __enable_irq();
}

void reset_all_channel_freq_min(void)
{
     __disable_irq();
    chx_info[0].freq_min = 0;
    chx_info[1].freq_min = 0;
    chx_info[2].freq_min = 0;
    chx_info[3].freq_min = 0;
    __enable_irq();
}

void reset_all_channel_calibrate(void)
{
    chx_info[0].calibrate = 0;
    chx_info[1].calibrate = 0;
    chx_info[2].calibrate = 0;
    chx_info[3].calibrate = 0;
}

bool is_chx_enable(channel_t chx)
{
    if (chx & active_channel_mask) {
        return true;
    } else {
        return false;
    }
}

uint8_t read_active_channel_mask(uint8_t channel_words[4])
{
    uint8_t mask = 0;
    if (channel_words[0] == 0xA1) {
        mask |= CH1;
    }
    if (channel_words[1] == 0xA2) {
        mask |= CH2;
    }
    if (channel_words[2] == 0xA3) {
        mask |= CH3;
    }
    if (channel_words[3] == 0xA4) {
        mask |= CH4;
    }
    LOG_I("active channel mask:0x%x", mask);
    return mask;
}

uint8_t get_active_channel_mask(void)
{
    return active_channel_mask;
}

bool is_all_channel_calibrate_finish(void)
{
    bool ret = true;
    for (int i = 0; i < CH_CNT; i++) {
        if (chx_info[i].calibrate != CALIBRATE_FINISH && is_chx_enable(1<<i)) {
            ret = false;
        }
    }
    return ret;
}

bool is_calibrate_any_channel_unknow_error(void)
{
    if (!is_all_channel_calibrate_finish()) {
        for (int i = 0; i < CH_CNT; i++) {
            if (chx_info[i].calibrate == CALIBRATE_ENZYME_DEACTIVATION ||
                chx_info[i].calibrate == CALIBRATE_VALUE_ERROR) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool is_calibrate_any_channel_known_error(void)
{
    if (!is_all_channel_calibrate_finish()) {
        for (int i = 0; i < CH_CNT; i++) {
            if (chx_info[i].calibrate == CALIBRATE_ENZYME_DEACTIVATION ||
                chx_info[i].calibrate == CALIBRATE_VALUE_ERROR) {
                return true;
            }
        }
        return false;
    } else {
        return false;
    }
}

void calibrate_data_process(channel_t chx)
{
    uint8_t ch_x = 0xff;
    for (int i = 0; i < CH_CNT; i++) {
        if ((1 << i) == chx) {
            ch_x = i;
        }
    }
    if (ch_x == 0xff) {
        LOG_I("input chx error!");
        return;
    }
    if (chx_info[ch_x].calibrate == CALIBRATE_INIT)
	{
        if (chx_info[ch_x].freq_diff < 8500000 && chx_info[ch_x].freq_diff <15000000) {
            /*定标结果判断  频率差小于10K即判断失活*/
            if (chx_info[ch_x].freq_diff > 5000) {
                chx_info[ch_x].calibrate_cnt++;
                if (chx_info[ch_x].calibrate_cnt == 1) {
                    chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                }
                if (chx_info[ch_x].calibrate_cnt == 2) {
                    /*通道一 浓度计算，精确到1*/
                    chx_info[ch_x].concentration = ((float)chx_info[ch_x].freq_diff/chx_info[ch_x].std_value)*100;
                    /*浓度值偏差大，重新设置分母*/
                    if (chx_info[ch_x].concentration <=96 || 103 <= chx_info[ch_x].concentration) {
                        /*通道一 本次定标不通过更新分母*/
                        chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                        chx_info[ch_x].calibrate_cnt = 1;
                    } else {
						/*定标完成，清除calibrate count*/
                        chx_info[ch_x].calibrate_cnt = 0;
                        chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                        chx_info[ch_x].calibrate = CALIBRATE_FINISH;
                    }
                }
            } else {
#ifdef CALIBRATE_ERROR_ENABLE
                chx_info[ch_x].calibrate_cnt = 0;
                /*通道一 酶膜失活 [预留]*/
                chx_info[ch_x].calibrate = CALIBRATE_ENZYME_DEACTIVATION;
#endif
            }
        } else {
#ifdef CALIBRATE_ERROR_ENABLE
            chx_info[ch_x].calibrate_cnt = 0;
            /*通道一 数值异常 [预留]*/
            chx_info[ch_x].calibrate = CALIBRATE_VALUE_ERROR;
#endif
        }
    } else {

    }
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

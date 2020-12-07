
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

#define bswap24(x) ((((x) & 0xff0000u) >> 16) | (((x) & 0x0000ffu) << 16) | ((x) & 0x00ff00u))
#define bswap16(x) ((((x) & 0xff00u) >> 8) | (((x) & 0x00ffu) << 8))

channel_context_t chx_info[4];

void SystemClock_Config(void);

extern uint8_t Start_Calculate;

uint8_t RX_Buf[14], RX_Flag, Order_Type;
uint8_t Feedback[44];

uint8_t active_channel_mask;

uint32_t LED_Count;

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

void dump_chx_info(void);

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
		/*receive command*/
        if (RX_Flag == 2) {
			RX_Flag = 0;
            hex_dump("received", RX_Buf, 14);
			/*select channel*/
			if (RX_Buf[3] == 0x37) {
                LOG_I("Channel select");
                active_channel_mask = 0;
                active_channel_mask |= read_active_channel_mask(&RX_Buf[4]);

                reset_all_channel_calibrate_count();
                reset_all_channel_freq_max();

                channel_timer_on_off(CH1|CH2|CH3|CH4, STOP);

                HAL_UART_Transmit(&huart2, RX_Buf, 14, 0xffff);
                LOG_I("Send RX buf back");
 			}
			/*select channel end*/

			/*receive calibrate and measure command*/
			else if (RX_Buf[3] == 0x33) {
                LOG_I("start to calibrate");
				Order_Type = 1;

                LOG_I("Start active channel timer");
                channel_timer_on_off(active_channel_mask, START);
				HAL_TIM_Base_Start_IT(&htim6);

                LOG_I("start basic timer 6");
				HAL_UART_Transmit(&huart2, RX_Buf, 14, 0xffff);
                LOG_I("Send RX buf back");
			}
            /*query for calibrate result*/
			else if (RX_Buf[3] == 0x34) {
				HAL_UART_Transmit(&huart2, Feedback, 44, 0xffff);
                LOG_I("upload feedback");
                dump_chx_info();
            }
            /*receive measure command*/
			else if (RX_Buf[3] == 0x35) {
                LOG_I("start to measure");
                reset_all_channel_calibrate();
                /*flag for measure state*/
				Order_Type=2;

                channel_timer_on_off(active_channel_mask, START);

				HAL_TIM_Base_Start_IT(&htim6);
                LOG_I("start basic timer 6");
				HAL_UART_Transmit(&huart2, RX_Buf, 14, 0xffff);
                LOG_I("Send RX buf back");
			}

            /*query for measure result*/
			else if(RX_Buf[3] == 0x36) {
				HAL_UART_Transmit(&huart2, Feedback, 44, 0xffff);
                LOG_I("upload feedback");
                dump_chx_info();
            } else {
                LOG_I("unknow command!");
            }

			/*receive calibrate and measure command end*/
		}

		/*Calibrate*/
		if ((Start_Calculate == 0x01) && (Order_Type == 1)) {
            LOG_I("process calibrate data...");

            __disable_irq();
			Start_Calculate=0;
            __enable_irq();
			Order_Type=0;

			if (active_channel_mask & CH1) {
                calibrate_data_process(CH1);
			}
			if (active_channel_mask & CH2) {
                calibrate_data_process(CH2);
			}
			if (active_channel_mask & CH3) {
                calibrate_data_process(CH3);
			}
			if (active_channel_mask & CH4) {
                calibrate_data_process(CH4);
			}
			/*judge calibrate result*/
            if (is_all_channel_calibrate_finish()) {
                LOG_I("calibrate sucess!!");
                reset_all_channel_calibrate();
                /*calibrate success:0x05, continue calibrate:0x01*/
                Feedback[2] = 0x05;
			}
			/*recive for error judge*/
#ifdef FEATURE_CALIBRATE_ERROR_ENABLE
			else if(is_calibrate_any_channel_unknow_error()) {
				Feedback[2] = 0x01;
                LOG_E("some channel unkown error");

            } else if (is_calibrate_any_channel_known_error()) {
				Feedback[2] = 0x00;
				Feedback[3] = chx_info[CH_1].calibrate;
				Feedback[4] = chx_info[CH_2].calibrate;
				Feedback[5] = chx_info[CH_3].calibrate;
				Feedback[6] = chx_info[CH_4].calibrate;
                LOG_E("some channel known error");

                reset_all_channel_calibrate();
			}
#else
			else {
				Feedback[2] = 0x01;
                LOG_E("Some problem, continuely calibrate");
            }
#endif

            Feedback[0] = 0x0a;
            Feedback[1] = 0x0c;

            Feedback[3] = 0x00;
            Feedback[4] = 0x00;
            Feedback[5] = 0x00;
            Feedback[6] = 0x00;

            Feedback[7] = 0x05;

            fill_calibrate_feedback_chx(&Feedback[8]);

            dump_chx_info();

            reset_all_channel_freq_max();
		}
		/*calibrate end*/

		/*concentration measure*/
        if ((Start_Calculate == 0x01) && (Order_Type == 2)) {
            LOG_I("process mesure data...");
            __disable_irq();
            Start_Calculate = 0;
            __enable_irq();
            Order_Type = 0;

            if (active_channel_mask & CH1) {
                update_concentration(CH_1);
            }

            if (active_channel_mask & CH2) {
                update_concentration(CH_2);
            }

            if(active_channel_mask & CH3) {
                update_concentration(CH_3);
            }

            if (active_channel_mask & CH4) {
                update_concentration(CH_4);
            }

            fill_concentration_feedback(&Feedback);
            fill_calibrate_feedback_chx(&Feedback[8]);

            dump_chx_info();

            reset_all_channel_freq_max();
		}
		/*concentration measure end*/

        LED_Count++;
        if (LED_Count % 4255999 == 0) {
            LED_Count = 0;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
        }
        HAL_IWDG_Refresh(&hiwdg);
    }
}

void dump_chx_info(void)
{
    for (int i = 0; i < CH_CNT; i++) {
        if (is_chx_enable(i+1)) {
            LOG_I("Channel_%d:", i+1);
            printf("  freq_max:%d\n", chx_info[i].freq_max);
            printf("  freq_min:%d\n", chx_info[i].freq_min);
            printf("  freq_diff:%d\n", chx_info[i].freq_diff);
            printf("  timer_cnt:%d\n", chx_info[i].timer_cnt);
            printf("  std_value:%d\n", chx_info[i].std_value);
            printf("  concentration:%d\n", chx_info[i].concentration);
            printf("  calibrate:%d\n", chx_info[i].calibrate);
            printf("  calibrate_cnt:%d\n", chx_info[i].calibrate_cnt);
            printf("\n");
        }
    }
}

static void update_concentration(uint8_t ch_x)
{
    chx_info[ch_x].freq_diff = chx_info[ch_x].freq_max - chx_info[ch_x].freq_min;
    chx_info[ch_x].concentration = ((float)chx_info[ch_x].freq_diff/chx_info[ch_x].std_value) * 1000;
    LOG_I("CH_%d concentration:[%d]", ch_x+1, chx_info[ch_x].concentration);
}

static void fill_calibrate_feedback_chx(uint8_t *feedback)
{
    calibrate_data_section_t data[4];
    for (int i = 0; i < CH_CNT; i++) {
        data[i].freq_max = bswap24(chx_info[i].freq_max);
        data[i].freq_min = bswap24(chx_info[i].freq_min);
        data[i].freq_diff = bswap24(chx_info[i].freq_diff);
        LOG_I("ch%d: freq_max:[%d]\tfreq_min:[%d]\tfreq_diff:[%d]", i+1, data[i].freq_max, data[i].freq_min, data[i].freq_diff);
    }
    memcpy(feedback, data, sizeof(calibrate_data_section_t) * 4);
    hex_dump("calibrate feedback:", feedback, sizeof(calibrate_data_section_t) * 4);
}

static void fill_concentration_feedback(uint8_t *feedback)
{
    concentration_data_t data;
    for (int i = 0; i < CH_CNT; i++) {
        data[i] = bswap16(chx_info[i].concentration);
    }
    memcpy(feedback, data, sizeof(concentration_data_t));
    hex_dump("concentration feedback:", feedback, sizeof(concentration_data_t));
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
    LOG_I("active channel mask:0x%02x", mask);
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
    LOG_I("Process CH_%d calibrate data, count:%d", ch_x+1, chx_info[ch_x].calibrate_cnt);
    if (chx_info[ch_x].calibrate == CALIBRATE_INIT)
	{
        if (chx_info[ch_x].freq_diff < 8500000 && chx_info[ch_x].freq_max < 15000000) {
            if (chx_info[ch_x].freq_diff > 10000) {
                chx_info[ch_x].calibrate_cnt++;
                if (chx_info[ch_x].calibrate_cnt == 1) {
                    LOG_I("calibrate cnt == 1, std_value = %d", chx_info[ch_x].freq_diff);
                    chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                }
                if (chx_info[ch_x].calibrate_cnt == 2) {
                    LOG_I("before calculate concentration, current freq_diff:%d, std_value:%d", chx_info[ch_x].freq_diff, chx_info[ch_x].std_value);
                    chx_info[ch_x].concentration = ((float)chx_info[ch_x].freq_diff/chx_info[ch_x].std_value)*100;
                    LOG_I("ch%d: calibrate cnt == 2, concentration = %f", chx, chx_info[ch_x].concentration);
                    if (chx_info[ch_x].concentration <= 96 || 103 <= chx_info[ch_x].concentration) {
                        LOG_I("ch%d difference between 2 freq_diff too large", chx);
                        chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                        chx_info[ch_x].calibrate_cnt = 1;
                    } else {
                        LOG_I("ch%d calibrate success", chx);
                        chx_info[ch_x].calibrate_cnt = 0;
                        chx_info[ch_x].std_value = chx_info[ch_x].freq_diff;
                        chx_info[ch_x].calibrate = CALIBRATE_FINISH;
                    }
                }
            } else {
#ifdef FEATURE_CALIBRATE_ERROR_ENABLE
                chx_info[ch_x].calibrate_cnt = 0;
                chx_info[ch_x].calibrate = CALIBRATE_ENZYME_DEACTIVATION;
                LOG_I("ENZYME_DEACTIVATION");
#endif
            }
        } else {
#ifdef FEATURE_CALIBRATE_ERROR_ENABLE
            chx_info[ch_x].calibrate_cnt = 0;
            chx_info[ch_x].calibrate = CALIBRATE_VALUE_ERROR;
            LOG_I("CALIBRATE_VALUE_ERROR");
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
     tex: LOG_I("Wrong parameters value: file %s on line %d\r\n", file, line) */
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

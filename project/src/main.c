
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

void SystemClock_Config(void);
extern volatile uint32_t Channel1_MAX,Channel2_MAX,Channel3_MAX,Channel4_MAX,Channel1_MIN,Channel2_MIN,Channel3_MIN,Channel4_MIN;
extern uint8_t Start_Calculate;
volatile uint32_t Difference_1,Difference_2,Difference_3,Difference_4;				//频率差
uint32_t StandardValue_1,StandardValue_2,StandardValue_3,StandardValue_4;			//定标分母
uint32_t Concentration_1,Concentration_2,Concentration_3,Concentration_4;			//浓度

uint8_t RX_Buf[14],RX_Flag,count_1=0,count_2=0,count_3=0,count_4=0,Order_Type,Enabled_1,Enabled_2,Enabled_3,Enabled_4;
uint8_t Feedback[44],Calibrate_1=0,Calibrate_2=0,Calibrate_3=0,Calibrate_4=0;				//定标及测样反馈数组

uint8_t active_channel_mask;

uint8_t LED_Mark;				//LED灯
uint32_t LED_Count;			//LED灯

uint8_t get_active_channel_mask(uint8_t channel_words[4]);
void channel_timer_on_off(channel_t chx, bool state);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
	
  MX_TIM2_Init();		//通道一
  MX_TIM3_Init();		//通道二
  MX_TIM1_Init();		//通道三
  MX_TIM8_Init();		//通道四
  MX_TIM6_Init();		//基础定时器 1秒
	
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
                active_channel_mask |= get_active_channel_mask(&RX_Buf[4]);
				count_1=0;
				count_2=0;
				count_3=0;
				count_4=0;
				Channel1_MAX=0;
				Channel2_MAX=0;
				Channel3_MAX=0;
				Channel4_MAX=0;

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
				Calibrate_1=0;
				Calibrate_2=0;
				Calibrate_3=0;
				Calibrate_4=0;

				Order_Type=2;															//测样标志位
				
                channel_timer_on_off(active_channel_mask, START);

				HAL_TIM_Base_Start_IT(&htim6); 									//开启定时器6 基础定时器
                LOG_I("start basic timer 6");
				HAL_UART_Transmit_DMA(&huart2,RX_Buf,14);						//通讯反馈					
                LOG_I("Send RX buf back");
			}
			
			else if( RX_Buf[3]==0x36 )												//测样结果查询54  0x36
            {
				HAL_UART_Transmit_DMA(&huart2,Feedback,44);					//进行测样反馈和频率上传
                LOG_I("upload feedback");
            }

			/********** 接收到定标、测样指令 结束 **********/
		
		}
		
		/************ 定标 开始 ************/
		if( (Start_Calculate==0x01) && (Order_Type==1) )						//定标数据处理	
		{
            LOG_I("定标数据处理...");

			Start_Calculate=0;
			Order_Type=0;
			
			if( (Enabled_1==1) && (Calibrate_1==0) )									//通道一 定标判断
			{
				Difference_1=Channel1_MAX-Channel1_MIN;								//取差值
				
				if( (Difference_1<8500000) && (Channel1_MAX<15000000) ) 			//定标结果判断  先取消了异常判断
				{
					if( Difference_1>5000 )																		//定标结果判断  频率差小于10K即判断失活 先取消了失活判定   
					{
						count_1++;
						if(count_1==1)
						{
							StandardValue_1=Difference_1;												//通道一 取频率差做分母
						}
						if(count_1==2)																							
						{
							Concentration_1=((float)Difference_1/StandardValue_1)*100;			//通道一 浓度计算，精确到1
							if( (Concentration_1<=96)||(103<=Concentration_1) )							//浓度值偏差大，重新设置分母
							{
								StandardValue_1=Difference_1;																	//通道一 本次定标不通过更新分母
								count_1=1;
							}
							else
							{
								count_1=0;															//定标完成，清除count
								StandardValue_1=Difference_1;
								Calibrate_1=1;
							}
						}
					}
//					else
//					{
//						count_1=0;
//						Calibrate_1=0xB1;							//通道一 酶膜失活 [预留]
//					}						
				}
//				else
//				{
//					count_1=0;
//					Calibrate_1=0xC1;								//通道一 数值异常 [预留]
//				}				
			}
			
			if( (Enabled_2==1) && (Calibrate_2==0) )									//通道二 定标判断
			{
				Difference_2=Channel2_MAX-Channel2_MIN;								//取差值
				
				if( (Difference_2<8500000) && (Channel2_MAX<15000000) ) 			//定标结果判断  先取消了异常判断
				{
					if( Difference_2>5000 )																//定标结果判断  频率差小于10K即判断失活 先取消了失活判定   
					{
						count_2++;	
						if(count_2==1)
						{
							StandardValue_2=Difference_2;												//通道二 取频率差做分母
						}
						if(count_2==2)																							
						{
							Concentration_2=((float)Difference_2/StandardValue_2)*100;		//通道二 浓度计算，精确到1
							if( (Concentration_2<=96)||(103<=Concentration_2) )				//浓度值偏差大，重新设置分母
							{
								StandardValue_2=Difference_2;											//通道二 本次定标不通过更新分母
								count_2=1;
							}
							else
							{
								count_2=0;																	//定标完成，清除count
								StandardValue_2=Difference_2;
								Calibrate_2=1;
							}
						}
					}
//					else
//					{
//						count_2=0;
//						Calibrate_2=0xB1;							//通道二 酶膜失活 [预留]
//					}						
				}
//				else
//				{
//					count_2=0;
//					Calibrate_2=0xC1;								//通道二 数值异常 [预留]
//				}				
			}			
			
			if( (Enabled_3==1) && (Calibrate_3==0) )									//通道三 定标判断
			{
				Difference_3=Channel3_MAX-Channel3_MIN;								//取差值
				
				if( (Difference_3<8500000) && (Channel3_MAX<15000000) ) 			//定标结果判断  先取消了异常判断
				{
					if( Difference_3>5000 )																//定标结果判断  频率差小于10K即判断失活 先取消了失活判定   
					{
						count_3++;
						if(count_3==1)
						{
							StandardValue_3=Difference_3;												//通道三 取频率差做分母
						}
						if(count_3==2)																							
						{
							Concentration_3=((float)Difference_3/StandardValue_3)*100;		//通道三 浓度计算，精确到1
							if( (Concentration_3<=96)||(103<=Concentration_3) )				//浓度值偏差大，重新设置分母
							{
								StandardValue_3=Difference_3;											//通道三 本次定标不通过更新分母
								count_3=1;
							}
							else
							{
								count_3=0;																	//定标完成，清除count
								StandardValue_3=Difference_3;
								Calibrate_3=1;
							}
						}
					}
//					else
//					{
//						count_3=0;
//						Calibrate_3=0xB1;							//通道三 酶膜失活 [预留]
//					}						
				}
//				else
//				{
//					count_3=0;
//					Calibrate_3=0xC1;								//通道三 数值异常 [预留]
//				}				
			}

			if( (Enabled_4==1) && (Calibrate_4==0) )									//通道四 定标判断
			{
				Difference_4=Channel4_MAX-Channel4_MIN;								//取差值
				
				if( (Difference_4<8500000) && (Channel4_MAX<15000000) ) 			//定标结果判断  先取消了异常判断
				{
					if( Difference_4>5000 )																//定标结果判断  频率差小于10K即判断失活 先取消了失活判定   
					{
						count_4++;
						if(count_4==1)
						{
							StandardValue_4=Difference_4;												//通道四 取频率差做分母
						}
						if(count_4==2)																							
						{
							Concentration_4=((float)Difference_4/StandardValue_4)*100;		//通道四 浓度计算，精确到1
							if( (Concentration_4<=96)||(103<=Concentration_4) )				//浓度值偏差大，重新设置分母
							{
								StandardValue_4=Difference_4;											//通道四 本次定标不通过更新分母
								count_4=1;
							}
							else
							{
								count_4=0;																	//定标完成，清除count
								StandardValue_4=Difference_4;
								Calibrate_4=1;
							}
						}
					}
//					else
//					{
//						count_4=0;
//						Calibrate_4=0xB1;							//通道四 酶膜失活 [预留]
//					}						
				}
//				else
//				{
//					count_4=0;
//					Calibrate_4=0xC1;								//通道四 数值异常 [预留]
//				}				
			}	
			
			/************************ 定标结果判断 **********************/
			if( ( ((Enabled_1==1)&&(Calibrate_1==1))||(Enabled_1==0) ) && 
			    ( ((Enabled_2==1)&&(Calibrate_2==1))||(Enabled_2==0) ) && 
				 ( ((Enabled_3==1)&&(Calibrate_3==1))||(Enabled_3==0) ) && 
				 ( ((Enabled_4==1)&&(Calibrate_4==1))||(Enabled_4==0) )  
			  )
			{
				Calibrate_1=0;
				Calibrate_2=0;
				Calibrate_3=0;
				Calibrate_4=0;
				Feedback[2]=0x05;															//定标通过――5		//继续定标是――1
			}
			/************************* 此为预留功能，勿删 *************************/
//			else if( (Calibrate_1!=0xB1) && (Calibrate_1!=0xC1) &&
//						(Calibrate_2!=0xB1) && (Calibrate_2!=0xC1) &&
//						(Calibrate_3!=0xB1) && (Calibrate_3!=0xC1) &&
//						(Calibrate_4!=0xB1) && (Calibrate_4!=0xC1) 			
//					 )
//				Feedback[2]=0x01;															//定标通过――5		//继续定标是――1
//			
//			else if( (Calibrate_1==0xB1) || (Calibrate_1==0xC1) ||
//						(Calibrate_2==0xB1) || (Calibrate_2==0xC1) ||
//						(Calibrate_3==0xB1) || (Calibrate_3==0xC1) ||
//						(Calibrate_4==0xB1) || (Calibrate_4==0xC1) 		
//					 )
//			{
//				Feedback[2]=0x00;
//				Feedback[3]=Calibrate_1;
//				Feedback[4]=Calibrate_2;
//				Feedback[5]=Calibrate_3;
//				Feedback[6]=Calibrate_4;
//				Calibrate_1=0;
//				Calibrate_2=0;
//				Calibrate_3=0;
//				Calibrate_4=0;
//			}

			else			//使用预留功能时，这两行注释掉
				Feedback[2]=0x01;															//定标通过――5		//继续定标是――1
			
			Feedback[0]=0x0a;
			Feedback[1]=0x0c;
			
			Feedback[3]=0x00;
			Feedback[4]=0x00;
			Feedback[5]=0x00;
			Feedback[6]=0x00;	
			
			Feedback[7]=0x05;										
			
			Feedback[8]=Channel1_MAX>>16;				//通道一 最大值上传
			Feedback[9]=Channel1_MAX>>8;				//最大值上传
			Feedback[10]=Channel1_MAX;					//最大值上传
			Feedback[11]=Channel1_MIN>>16;			//通道一 最小值上传
			Feedback[12]=Channel1_MIN>>8;				//最小值上传
			Feedback[13]=Channel1_MIN;					//最小值上传
			Feedback[14]=Difference_1>>16;			//通道一 差值上传
			Feedback[15]=Difference_1>>8;				//差值上传
			Feedback[16]=Difference_1;					//差值上传
			
			Feedback[17]=Channel2_MAX>>16;			//通道二 最大值上传
			Feedback[18]=Channel2_MAX>>8;				//最大值上传
			Feedback[19]=Channel2_MAX;					//最大值上传
			Feedback[20]=Channel2_MIN>>16;			//通道二 最小值上传
			Feedback[21]=Channel2_MIN>>8;				//最小值上传
			Feedback[22]=Channel2_MIN;					//最小值上传
			Feedback[23]=Difference_2>>16;			//通道二 差值上传
			Feedback[24]=Difference_2>>8;				//差值上传
			Feedback[25]=Difference_2;					//差值上传
		
			Feedback[26]=Channel3_MAX>>16;			//通道三 最大值上传
			Feedback[27]=Channel3_MAX>>8;				//最大值上传
			Feedback[28]=Channel3_MAX;					//最大值上传
			Feedback[29]=Channel3_MIN>>16;			//通道三 最小值上传
			Feedback[30]=Channel3_MIN>>8;				//最小值上传
			Feedback[31]=Channel3_MIN;					//最小值上传
			Feedback[32]=Difference_3>>16;			//通道三 差值上传
			Feedback[33]=Difference_3>>8;				//差值上传
			Feedback[34]=Difference_3;					//差值上传
			
			Feedback[35]=Channel4_MAX>>16;			//通道四 最大值上传
			Feedback[36]=Channel4_MAX>>8;				//最大值上传
			Feedback[37]=Channel4_MAX;					//最大值上传
			Feedback[38]=Channel4_MIN>>16;			//通道四 最小值上传
			Feedback[39]=Channel4_MIN>>8;				//最小值上传
			Feedback[40]=Channel4_MIN;					//最小值上传
			Feedback[41]=Difference_4>>16;			//通道四 差值上传
			Feedback[42]=Difference_4>>8;				//差值上传
			Feedback[43]=Difference_4;					//差值上传
			
			Channel1_MAX=0;
			Channel2_MAX=0;
			Channel3_MAX=0;
			Channel4_MAX=0;

		}
		/************ 定标 结束 ************/
		
		/************ 浓度检测 开始 ************/
		if( (Start_Calculate==0x01) && (Order_Type==2) )						//检测数据处理
		{
            LOG_I("检测数据处理...");
			Start_Calculate=0;
			Order_Type=0;
			
			if(Enabled_1==1)				//通道一开启
			{
				Difference_1=Channel1_MAX-Channel1_MIN;
				Concentration_1=((float)Difference_1/StandardValue_1)*1000;			//通道一浓度计算，精确到0.1位
			}
			
			if(Enabled_2==1)				//通道二开启
			{
				Difference_2=Channel2_MAX-Channel2_MIN;
				Concentration_2=((float)Difference_2/StandardValue_2)*1000;			//通道二浓度计算，精确到0.1位
			}
			
			if(Enabled_3==1)				//通道三开启
			{
				Difference_3=Channel3_MAX-Channel3_MIN;
				Concentration_3=((float)Difference_3/StandardValue_3)*1000;			//通道三浓度计算，精确到0.1位
			}

			if(Enabled_4==1)				//通道四开启
			{
				Difference_4=Channel4_MAX-Channel4_MIN;
				Concentration_4=((float)Difference_4/StandardValue_4)*1000;			//通道四浓度计算，精确到0.1位
			}
			
			
			Feedback[0]=Concentration_1>>8;			 //通道一 浓度
			Feedback[1]=Concentration_1;				  //通道一 浓度
			Feedback[2]=Concentration_2>>8;			 //通道二 浓度
			Feedback[3]=Concentration_2;				  //通道二 浓度
			Feedback[4]=Concentration_3>>8;			 //通道三 浓度
			Feedback[5]=Concentration_3;				  //通道三 浓度
			Feedback[6]=Concentration_4>>8;			 //通道四 浓度
			Feedback[7]=Concentration_4;				  //通道四 浓度
			
			Feedback[8]=Channel1_MAX>>16;				//通道一 最大值上传
			Feedback[9]=Channel1_MAX>>8;				//最大值上传
			Feedback[10]=Channel1_MAX;					//最大值上传
			Feedback[11]=Channel1_MIN>>16;			//通道一 最小值上传
			Feedback[12]=Channel1_MIN>>8;				//最小值上传
			Feedback[13]=Channel1_MIN;					//最小值上传
			Feedback[14]=Difference_1>>16;			//通道一 差值上传
			Feedback[15]=Difference_1>>8;				//差值上传
			Feedback[16]=Difference_1;					//差值上传
			
			Feedback[17]=Channel2_MAX>>16;			//通道二 最大值上传
			Feedback[18]=Channel2_MAX>>8;				//最大值上传
			Feedback[19]=Channel2_MAX;					//最大值上传
			Feedback[20]=Channel2_MIN>>16;			//通道二 最小值上传
			Feedback[21]=Channel2_MIN>>8;				//最小值上传
			Feedback[22]=Channel2_MIN;					//最小值上传
			Feedback[23]=Difference_2>>16;			//通道二 差值上传
			Feedback[24]=Difference_2>>8;				//差值上传
			Feedback[25]=Difference_2;					//差值上传
		
			Feedback[26]=Channel3_MAX>>16;			//通道三 最大值上传
			Feedback[27]=Channel3_MAX>>8;				//最大值上传
			Feedback[28]=Channel3_MAX;					//最大值上传
			Feedback[29]=Channel3_MIN>>16;			//通道三 最小值上传
			Feedback[30]=Channel3_MIN>>8;				//最小值上传
			Feedback[31]=Channel3_MIN;					//最小值上传
			Feedback[32]=Difference_3>>16;			//通道三 差值上传
			Feedback[33]=Difference_3>>8;				//差值上传
			Feedback[34]=Difference_3;					//差值上传
			
			Feedback[35]=Channel4_MAX>>16;			//通道四 最大值上传
			Feedback[36]=Channel4_MAX>>8;				//最大值上传
			Feedback[37]=Channel4_MAX;					//最大值上传
			Feedback[38]=Channel4_MIN>>16;			//通道四 最小值上传
			Feedback[39]=Channel4_MIN>>8;				//最小值上传
			Feedback[40]=Channel4_MIN;					//最小值上传
			Feedback[41]=Difference_4>>16;			//通道四 差值上传
			Feedback[42]=Difference_4>>8;				//差值上传
			Feedback[43]=Difference_4;					//差值上传			
			
			Channel1_MAX=0;
			Channel2_MAX=0;
			Channel3_MAX=0;
			Channel4_MAX=0;
			
		}
		
		
		
		/************ 浓度检测 结束 ************/
		
		
		/********* LED灯闪烁 **********/
		LED_Count++;
		if(LED_Count%4255999==0)
		{
			LED_Count=0;
			if(LED_Mark==0)
			{
				LED_Mark=1;
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
			}
			else
			{
				LED_Mark=0;
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
			}
		}	
		
		HAL_IWDG_Refresh(&hiwdg);			//IWDG看门狗喂狗
		
  }

}

uint8_t get_active_channel_mask(uint8_t channel_words[4])
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

void channel_timer_on_off(channel_t chx, bool state)
{
    int i = 0;
    channel_t channel_index = 1;
    LOG_I("channel mask:0x%x set state:%d", chx, state);
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

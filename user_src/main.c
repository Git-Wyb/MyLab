/**
  ******************************************************************************
  * @file    Project/STM8L15x_StdPeriph_Template/main.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    13-May-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <iostm8l151g4.h> // CPU型号
#include "Pin_define.h"   // 管脚定义
#include "initial.h"      // 初始�?  预定�?
#include "ram.h"          // RAM定义
#include "ADF7030_1.h"    // 初始化ADF7021
#include "Timer.h"        // 定时�?
#include "ID_Decode.h"    // ID_Decode处理
#include "eeprom.h"       // eeprom
#include "uart.h"         // uart
/** @addtogroup STM8L15x_StdPeriph_Template
  * @{
  */
/* Private typedef -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

void main(void)
{
    u8 Flag_Lower_Limit = 0;
    u8 Flag_Abnormal = 0;
    u8 flag_abnormal_stats = 0;  //异常信号标志
    u8 Abnormal_cnt = 0;
    u8 Lower_Limit_cnt = 0;
    u8 time_tx = 0;
    u8 Time_Beep_0n = 0;
    u8 Time_Beep_0ff = 0;

    _DI();             // 关全�?中断
    RAM_clean();       // 清除RAM

    VHF_GPIO_INIT();   //IO初始�?
    SysClock_Init();   //系统时钟初始�?
    InitialFlashReg(); //flash EEPROM
    eeprom_sys_load(); //ID载入
    TIM4_Init();       // 定时�?
    //beep_init();       // 蜂鸣�?
    ClearWDT(); // Service the WDT

    PROFILE_CH_FREQ_32bit_200002EC = 426075000;
    PROFILE_RADIO_AFC_CFG1_32bit_2000031C = 0x0005005A;
    PROFILE_RADIO_DATA_RATE_32bit_200002FC = 0x6400000C;
    //PROFILE_GENERIC_PKT_FRAME_CFG1_32bit_20000500 = 0x0000100C;
    ADF7030Init(); //射频初始�?
    UART1_INIT();  // UART1 for PC Software
    _EI();         // 允许中断
    ClearWDT(); // Service the WDT
    RF_test_mode();
    TIME_power_led = 500;
    WDT_init();        //看门狿
    //  FLAG_APP_RX = 1;
    FG_Receiver_LED_RX = 0;
    //TIME_EMC = 10;
    FLAG_testNo91 = 0;
    FLAG_testBEEP = 0;

    if(Abnormal_Signal)    Flag_Abnormal = 1;
    if(Lower_Limit_Signal)   Flag_Lower_Limit = 1;

    while (1)
    {
        ClearWDT(); // Service the WDT
        //if (FLAG_testBEEP != 0)
        //    TEST_beep();

        if (time_Login_exit_256 == 0)
            ID_Decode_OUT();
        if (FG_10ms)
        {
            if(time_tx) --time_tx;
            if(Time_Beep_0n)   --Time_Beep_0n;
            if(Time_Beep_0ff)  --Time_Beep_0ff;
            ID_learn();
        }
    //if ((ID_SCX1801_DATA != 0) && (Receiver_426MHz_mode == 0))
        if((ID_SCX1801_DATA != 0) && Receiver_429MHz_mode == 0 && Flag_ID_Login != 1)//有ID登录且不是万能码遥控就发送状态
        {
            APP_TX_PACKET();
        }
        if(Flag_ID_Login == 1) //接收到特殊ID并且有ID登录就启动蜂鸣器
        {
            if(Time_Beep_0n == 0)
            {
                if (FG_beep_on == 0)
                {
                    FG_beep_on = 1;
                    FG_beep_off = 0;
                    Time_Beep_0ff = 10; //100ms
                    TIM3_init();
                }
                ClearWDT();   // Service the WDT
            }
            if(Time_Beep_0ff == 0)
            {
                if (FG_beep_off == 0)
                {
                    FG_beep_off = 1;
                    FG_beep_on = 0;
                    Time_Beep_0n = 5;   //50ms
                    Tone_OFF();
                }
                ClearWDT();   // Service the WDT
            }
        }
        if (FLAG_APP_RX == 1)
        {
            Freq_Scanning();
            //if(Scan_step==2)
            SCAN_RECEIVE_PACKET(); //ɨ���������?
        }
     //   TranmissionACK();
    //        READ_RSSI_avg();

        if (FG_Receiver_LED_RX == 1)
            Receiver_LED_RX = 1;
        else if (FG_Receiver_LED_RX == 0)
            Receiver_LED_RX = 0;

        /*必须有ID登录才进行异常、下限检测，异常、下限信号有变化时则计时1s之后再次判断，若有变化则发送状态
          若通过APP操作的则以APP状态指令返回，遥控器操作则以遥控器状态指令返回*/
        if(ID_SCX1801_DATA && Flag_Abnormal != Abnormal_Signal && time_tx == 0)  //异常信号
        {
            Abnormal_cnt ++;
            time_tx = 100;
            if(Abnormal_cnt == 2)  //异常有变化
            {
                Flag_Abnormal = Abnormal_Signal;//保存变化状态

                if(Abnormal_Signal == 0)
                {
                    flag_abnormal_stats = 1;   //有异常
                    if(PROFILE_RxLowSpeed_TYPE == 1)   //APP
                    {
                        Struct_DATA_Packet_Contro_fno = APP_Abnormal_State;
                    }
                    else if(PROFILE_RxLowSpeed_TYPE == 2)  //遥控器
                    {
                        Struct_DATA_Packet_Contro_fno = STX_Abnormal_State;
                    }
                }
                else
                {
                    flag_abnormal_stats = 0;  //异常消失
                    if(Lower_Limit_Signal == 0)
                    {
                        if(PROFILE_RxLowSpeed_TYPE == 1)
                        {
                            Struct_DATA_Packet_Contro_fno = APP_Close_State;
                        }
                        else if(PROFILE_RxLowSpeed_TYPE == 2)
                        {
                            Struct_DATA_Packet_Contro_fno = STX_Close_State;
                        }
                    }
                    else
                    {
                        if(PROFILE_RxLowSpeed_TYPE == 1)
                        {
                            Struct_DATA_Packet_Contro_fno = APP_Open_State;
                        }
                        else if(PROFILE_RxLowSpeed_TYPE == 2)
                        {
                            Struct_DATA_Packet_Contro_fno = STX_Open_State;
                        }
                    }
                }
                Abnormal_cnt = 0;
                time_sw = 500;  //开启发送
            }
        }
        else if(ID_SCX1801_DATA && Flag_Lower_Limit != Lower_Limit_Signal && time_tx == 0)  //下限信号
        {
            Lower_Limit_cnt ++;
            time_tx = 100;
            if(Lower_Limit_cnt == 2)  //下限信号有变化
            {
                Flag_Lower_Limit = Lower_Limit_Signal;

                if(flag_abnormal_stats == 1)  //存在异常仍然返回异常
                {
                    if(PROFILE_RxLowSpeed_TYPE == 1)   //APP
                    {
                        Struct_DATA_Packet_Contro_fno = APP_Abnormal_State;
                    }
                    else if(PROFILE_RxLowSpeed_TYPE == 2)  //遥控器
                    {
                        Struct_DATA_Packet_Contro_fno = STX_Abnormal_State;
                    }
                }
                else if(Lower_Limit_Signal == 0)
                {
                    if(PROFILE_RxLowSpeed_TYPE == 1)
                    {
                        Struct_DATA_Packet_Contro_fno = APP_Close_State;
                    }
                    else if(PROFILE_RxLowSpeed_TYPE == 2)
                    {
                        Struct_DATA_Packet_Contro_fno  = STX_Close_State;
                    }
                }
                else
                {
                    if(PROFILE_RxLowSpeed_TYPE == 1)
                    {
                        Struct_DATA_Packet_Contro_fno = APP_Open_State;
                    }
                    else if(PROFILE_RxLowSpeed_TYPE == 2)
                    {
                        Struct_DATA_Packet_Contro_fno =  STX_Open_State;
                    }
                }
                Lower_Limit_cnt = 0;
                time_sw = 500;
            }
        }
        if(Flag_Abnormal == Abnormal_Signal)
        {
            Abnormal_cnt = 0;
        }
        if(Flag_Lower_Limit == Lower_Limit_Signal)
        {
            Lower_Limit_cnt = 0;
        }
    }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

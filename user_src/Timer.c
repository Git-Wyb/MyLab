/***********************************************************************/
/*  FILE        :Timer.c                                               */
/*  DATE        :Mar, 2014                                             */
/*  Programmer	:xiang 'R                                              */
/*  CPU TYPE    :STM8L151G6     Crystal: 16M HSI                       */
/*  DESCRIPTION :                                                      */
/*  Mark        :ver 1.0                                               */
/***********************************************************************/
#include <iostm8l151g4.h> // CPU型号
#include "Pin_define.h"   // 管脚定义
#include "initial.h"      // 初始�? 预定�?
#include "ram.h"          // RAM定义
#include "uart.h"
u16 ErrStateTimeer = 1;
u16 StateReadTimer = 500;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%Timer 4 start   1ms
void TIM4_Init(void)
{
    TIM4_PSCR = 0x06; // Timer 4 prescaler  计数器时钟频�? f CK_CNT  =f CK_PSC  / 2的N次方
                      //TIM4_PSCR = 0x08;	// Timer 4 prescaler  计数器时钟频�? f CK_CNT  = f CK_PSC/ 2(PSC[3:0])
    TIM4_ARR = 0xF9;  // Timer 4 period
    TIM4_CR1 |= 0x01; // Timer 4 Enable
    TIM4_IER |= 0x01; // Timer 4 OVR interrupt
}

void TIM4_UPD_OVF(void)
{ //725==1�?
    if (TIMER1s)
    {
        --TIMER1s;
        if(Status_Un.PROFILE_RxLowSpeed_TYPE == 1)    //429M
        {
            if(TIMER1s == 1600)       //1600
            {
                Flag_429M_EndStop = 1;
            }
            else if(TIMER1s == 1100)     //1100
            {
                Flag_429M_EndStop = 2;
            }
        }
    }
	if(TIME_TX_RSSI_Scan)
		--TIME_TX_RSSI_Scan;
	if(TIME_power_led)
		--TIME_power_led;
    if (TIMER300ms)
        --TIMER300ms;
    if (TIMER18ms)
        --TIMER18ms;
    if (TIMER250ms_STOP)
        --TIMER250ms_STOP;
    if (TIME_10ms)
        --TIME_10ms;
    else
    { // 10mS FLAG
        TIME_10ms = 10;
        FG_10ms = 1;
        if(Time_Check_AutoSignal)
            --Time_Check_AutoSignal;
        if(Time_NoCheck_AutoSignal)
            --Time_NoCheck_AutoSignal;
        else if (TIME_auto_out)
            --TIME_auto_out;
        if(time_receive_auto)
            --time_receive_auto;
        if(Time_StateDetection)
            --Time_StateDetection;
    }
    if (U1AckTimer)
        U1AckTimer--;
    if (Time_APP_RXstart)
      --Time_APP_RXstart;
    if(Time_APP_blank_TX)
       --Time_APP_blank_TX;
    if (Flag_RSSI_Read_Timer)
        Flag_RSSI_Read_Timer--;
    if (X_ERRTimer)
        X_ERRTimer--;
	if (TIME_ID_SCX1801_Login)
		--TIME_ID_SCX1801_Login;
    if(time_sw)   --time_sw;

    TIM4_SR1_bit.UIF = 0; // 清除中断标记
}

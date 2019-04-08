/***********************************************************************/
/*  FILE        :ID_Decode.c                                           */
/*  DATE        :Mar, 2014                                             */
/*  Programmer	:xiang 'R                                              */
/*  CPU TYPE    :STM8L151G6     Crystal: 16M HSI                       */
/*  DESCRIPTION :                                                      */
/*  Mark        :ver 1.0                                               */
/***********************************************************************/
#include <iostm8l151g4.h> // CPU型号
#include "Pin_define.h"   // 管脚定义
#include "initial.h"      // 初�?��?? 预定�?
#include "ram.h"          // RAM定义
#include "ID_Decode.h"
#include "eeprom.h" // eeprom
#include "uart.h"   // uart
#include "ADF7030_1.h"
//void EXIT_init(void)
//{
//    EXTI_CR1 = 0x20;          //PORT B2  的中�?触发�?
//    ADF7021_DATA_CLK_CR2 = 1; //使能�?I/O口中�? PA1
//    //   EXTI_CR2=0X00;
//    //   PIN_PD7_CR2=1;
//}
/**
 ****************************************************************************
 * @Function : void DataReceive(void)
 * @File     : ID_Decode.c
 * @Program  :
 * @Created  : 2017/5/5 by Xiaowine
 * @Brief    : TEST BER
 * @Version  : V1.0
**/
void DataReceive(void)
{
    static u8 StateCache = 0;
    static u8 Cache = 0;
    static u8 X_HISbyte = 0;
    switch (StateCache)
    {
    case 0:
    {
        Cache <<= 1;
        if (ADF7030DATA)
            Cache++;
        if (Cache == 0x55)
        {
            StateCache = 1;
            X_HISbyte = 0;
            Cache = 0;
        }
    }
    break;
    case 1:
    {
        if (ADF7030DATA != X_HISbyte)
            X_ERR++;
        X_COUNT++;
        X_HISbyte ^= 1;
        if (X_COUNT >= 1000)
            StateCache = 2;
    }
    break;
    case 2:
        if (X_COUNT == 0)
            StateCache = 0;
        break;
    default:
        break;
    }
    EXTI_SR1_P4F = 1;
}

void ID_Decode_IDCheck(void)
{

    if (FLAG_Receiver_IDCheck)
    {
        FLAG_Receiver_IDCheck = 0;
        if(Radio_Date_Type==1) Signal_DATA_Decode(0);
		else if(Radio_Date_Type==2) Signal_DATA_Decode(2);
        if (FLAG_Signal_DATA_OK == 1)
        {
            Radio_Date_Type_bak=Radio_Date_Type;
            eeprom_IDcheck();
            if ((FLAG_ID_Erase_Login == 1) || (FLAG_ID_Login == 1) ||(FLAG_ID_SCX1801_Login==1))
            {
                if ((FLAG_ID_Login_OK == 0) && (DATA_Packet_Contro_buf != 0x40) && (DATA_Packet_ID != 0)&&(Radio_Date_Type_bak==1)) //2015.4.1�?�? 在登录模式下 不允许自动�?�信登录，只允�?�手动送信登录
                {
                    if ((FLAG_ID_Erase_Login == 1) || (((FLAG_ID_Login == 1) ||(FLAG_ID_SCX1801_Login==1))&&(DATA_Packet_ID != 0xFFFFFE)))
                    {
                        FLAG_ID_Login_OK = 1;
	                    ID_Receiver_Login = DATA_Packet_ID;
                    }
                }
            }
            else if ((FLAG_IDCheck_OK == 1) || (DATA_Packet_ID == 0xFFFFFE))
            {
                FLAG_IDCheck_OK = 0;
			    if(Radio_Date_Type_bak==1)
			    {
                    if(PROFILE_CH_FREQ_32bit_200002EC == 429175000 || PROFILE_CH_FREQ_32bit_200002EC == 429200000)   // APP
                    {
                        PROFILE_RxLowSpeed_TYPE = 1;    // �?�?429低速才能发�?
                    }
                    else if(PROFILE_CH_FREQ_32bit_200002EC == 426075000)   //STX
                    {
                        PROFILE_RxLowSpeed_TYPE = 2;
                    }
                        if (DATA_Packet_ID == 0xFFFFFE)
		                    DATA_Packet_Control = DATA_Packet_Contro_buf; //2015.3.24�?�? Control缓存�?ID判断�?否�?�习过后才能使用

		                if ((SPI_Receive_DataForC[1] & 0x0000FFFF) == 0x5556)
		                {
		                    PAYLOAD_SIZE = RX_PayLoadSizeLogin;
							Flag_TX_ID_load=1;
		                    Signal_DATA_Decode(1);
		                    if (FLAG_Signal_DATA_OK == 1)
		                    {
		                        eeprom_IDcheck();
		                        if (DATA_Packet_Contro_buf == 0xFF)
		                        { //2015.4.2�?�?  送信器旁边的登录�?追加登录不�??
		                            if (FLAG_IDCheck_OK == 1)
		                                FLAG_IDCheck_OK = 0;
		                            else if (ID_DATA_PCS < ID_Max_PCS)
		                            {
		                                BEEP_and_LED();
		                                ID_Receiver_Login = DATA_Packet_ID;
		                                ID_EEPROM_write();
		                            } //end else
		                        }
		                        else if (DATA_Packet_Control == 0x00)
		                        {
		                            if (FLAG_IDCheck_OK == 1)
		                            {
		                                FLAG_IDCheck_OK = 0;
		                                BEEP_and_LED();
										if(ID_SCX1801_DATA==DATA_Packet_ID)
                                        {
                                            ID_SCX1801_DATA = 0;
                                            ID_SCX1801_EEPROM_write(0x00);
                                            if (ID_Receiver_DATA[0] != 0)
                                            {
                                                ID_SCX1801_DATA = ID_Receiver_DATA[0];
                                                ID_SCX1801_EEPROM_write(ID_SCX1801_DATA);
                                                Delete_GeneralID_EEPROM(ID_SCX1801_DATA);
                                            }
                                        }
                                        else
                                            Delete_GeneralID_EEPROM(DATA_Packet_ID);
		                            }
		                        }
		                    }
		                }
		                else
		                {
		                    PAYLOAD_SIZE = RX_PayLoadSizeNOLogin;
							Flag_TX_ID_load=0;
		                    if ((DATA_Packet_Control == 0x40) && (Manual_override_TIMER == 0))
		                    {
		                        FG_auto_manual_mode = 1;
                                switch(DIP_SW_Code())
                                {
                                    case SW_CODE_0:
                                         TIME_auto_out = 890;
                                    break;
                                    case SW_CODE_1:
                                         TIME_auto_out = 890 * 2;
                                    break;
                                    case SW_CODE_2:
                                         TIME_auto_out = 890 * 3;
                                    break;
                                    case SW_CODE_3:
                                         TIME_auto_out = 890 * 4;
                                    break;
                                    case SW_CODE_4:
                                         TIME_auto_out = 890 * 5;
                                    break;
                                    case SW_CODE_5:
                                         TIME_auto_out = 890 * 6;
                                    break;
                                    case SW_CODE_6:
                                         TIME_auto_out = 890 * 7;
                                    break;
                                    case SW_CODE_7:
                                         TIME_auto_out = 890 * 8;
                                    break;
                                    case SW_CODE_8:
                                         TIME_auto_out = 890 * 9;
                                    break;
                                    case SW_CODE_9:
                                         TIME_auto_out = 890 * 10;
                                    break;
                                    default:
                                    break;
                                }
		                        //TIME_auto_out = 890; // 900
		                        if (FG_First_auto == 0)
		                        {
		                            FG_First_auto = 1;
		                            TIMER1s = 3000; //2500
		                        }
		                    }
		                    else if (DATA_Packet_Control == 0x40)
		                        ;
		                    else
		                    {
		                        FG_auto_out = 0;
		                        TIME_auto_close = 0;
		                        FG_auto_open_time = 0;
		                        if (FG_auto_manual_mode == 1)      //Manual_override_TIMER=13500;   //2�?0秒自动无�?
		                            Manual_override_TIMER = 24480; //4�?0秒自动无�?
		                        if ((DATA_Packet_Control & 0x14) == 0x14)
		                        {
		                            if (TIMER1s == 0)
		                                TIMER1s = 3800 - 30;
		                        }
		                        else if(FLAG_testNo91==1) TIMER1s = 600;
		                        else TIMER1s = 1000;
		                    }
		                    TIMER300ms = 600;  //500
		                    //Receiver_LED_RX=1;
		                    FG_Receiver_LED_RX = 1;
		                }
			    }
				else if(Radio_Date_Type_bak==2)
				{
				   DATA_Packet_Control=0;
				   Struct_DATA_Packet_Contro_fno=Struct_DATA_Packet_Contro.Fno_Type.UN.fno;
				   if(Struct_DATA_Packet_Contro.Fno_Type.UN.type==1) DATA_Packet_Control=Struct_DATA_Packet_Contro.data[0].uc[0];
					if(((DATA_Packet_Control&0xA0)==0x20)||((DATA_Packet_Control&0xC0)==0x40))TIMER1s=500;
					else if(((DATA_Packet_Control&0xDF)>0x80)&&((DATA_Packet_Control&0x20)==0x00)){
						TIMER1s=1000;//(TIMER_Semi_open+1)*1000;
						if((DATA_Packet_Control&0xDF)<0xC0)TIMER_Semi_open=(DATA_Packet_Control&0x1F)+4;
						else TIMER_Semi_close=(DATA_Packet_Control&0x1F)+4;
					}
					else TIMER1s=1000;
					FLAG_APP_TX_once=1;
                    TIMER300ms = 100;
		            FG_Receiver_LED_RX = 1;
				}
            }


        }
    }
}

void Signal_DATA_Decode(UINT8 NUM_Type)
{
    UINT32 data_in;
    UINT16 data_out;
    UINT16 data_NRZ[7];
    UINT8 i, j,i_value;

	if((NUM_Type==0)||(NUM_Type==1))i_value=3;
	else if(NUM_Type==2)i_value=7;
    for (i = 0; i < i_value; i++)
    {
        if ((NUM_Type == 0)||(NUM_Type == 2))
            data_in = SPI_Receive_DataForC[i];
        else
            data_in = SPI_Receive_DataForC[i + 3];
        data_out = 0;
        data_in = data_in >> 1;
        for (j = 0; j < 16; j++)
        {
            data_out = data_out << 1;
            if (data_in & 0x00000001)
                data_out += 1;
            data_in = data_in >> 2;
        }
        data_NRZ[i] = data_out;
    }
	if((NUM_Type==0)||(NUM_Type==1))
	{
	    if (data_NRZ[2] == ((data_NRZ[0] + data_NRZ[1]) & 0xFFFF))
	    {
	        FLAG_Signal_DATA_OK = 1;
	        LCDUpdateIDFlag = 1;
	        DATA_Packet_ID = (data_NRZ[1] & 0x00FF) * 65536 + data_NRZ[0];
	        if (DATA_Packet_ID == 0)
	            FLAG_Signal_DATA_OK = 0;                          //2014.3.21追加  不允许使用ID=0
	        DATA_Packet_Contro_buf = (data_NRZ[1] & 0xFF00) >> 8; //2015.3.24�?�? Control缓存�?ID判断�?否�?�习过后才能�?
	    }
	    else
	        FLAG_Signal_DATA_OK = 0;
	}
	else if(NUM_Type==2)
	{
	    if (data_NRZ[6] == ((data_NRZ[0] + data_NRZ[1] + data_NRZ[2] + data_NRZ[3] + data_NRZ[4] + data_NRZ[5]) & 0xFFFF))
	    {
	        FLAG_Signal_DATA_OK = 1;
	        LCDUpdateIDFlag = 1;
	        DATA_Packet_ID = (data_NRZ[1] & 0x00FF) * 65536 + data_NRZ[0];
	        if (DATA_Packet_ID == 0)
	            FLAG_Signal_DATA_OK = 0;                          //2014.3.21追加  不允许使用ID=0
	        Struct_DATA_Packet_Contro_buf.Fno_Type.byte = (data_NRZ[1] & 0xFF00) >> 8; //2015.3.24�?�? Control缓存�?ID判断�?否�?�习过后才能�?
            for (i = 0; i < 4; i++)
				Struct_DATA_Packet_Contro_buf.data[i].ui=data_NRZ[i+2];
	    }
	    else
	        FLAG_Signal_DATA_OK = 0;
	}
}

void eeprom_IDcheck(void)
{
    UINT16 i;
	if(FLAG_testNo91==1)
		{
            FLAG_IDCheck_OK = 1;
            DATA_Packet_Control = DATA_Packet_Contro_buf;
		}
#ifndef DEF_test_MAX_32pcs
		if(Radio_Date_Type_bak==1)
		{
				i = 0;
                do
				{
					if (ID_Receiver_DATA[i] == DATA_Packet_ID)
					{
						INquiry = i;
						i = ID_DATA_PCS;
						FLAG_IDCheck_OK = 1;
						if(Radio_Date_Type_bak==1)DATA_Packet_Control = DATA_Packet_Contro_buf;
						else if(Radio_Date_Type_bak==2)Struct_DATA_Packet_Contro=Struct_DATA_Packet_Contro_buf;
					} //2015.3.24?0?1?0?7?0?3?0?3?0?2?0?5 Control?0?4?0?4��?0?2?0?2?0?0��?0?8?ID?0?2?0?3��?0?3�C?0?2?0?3?0?0?0?4?0?2?0?6?0?7?0?2?0?2?0?7?0?1?0?1?0?2��?0?7?6?0?0?2?0?6?0?5?0?3��?0?3��?0?6?0?5?0?1?0?5?0?7?0?4����
                    else if (DATA_Packet_ID == ID_SCX1801_DATA)
                    {
                        i = ID_DATA_PCS;
                        FLAG_IDCheck_OK = 1;
                        DATA_Packet_Control = DATA_Packet_Contro_buf;
                    }
					if ((FLAG_ID_Erase_Login == 1) && (FLAG_ID_Erase_Login_PCS == 1))
					{
						i = ID_DATA_PCS;
						FLAG_IDCheck_OK = 0;
						DATA_Packet_Control = DATA_Packet_Contro_buf;
					} //��?0?7?0?5?0?2?0?7?0?2?0?2��?0?8?0?3?0?1?0?3ID?0?4?6?4?0?3?0?2?0?5?6?1
                    i++;
                } while (i < ID_DATA_PCS);
		}
		else if((Radio_Date_Type_bak==2)&&(DATA_Packet_ID==ID_SCX1801_DATA))
		{
			FLAG_IDCheck_OK = 1;
			Struct_DATA_Packet_Contro=Struct_DATA_Packet_Contro_buf;
		}
#else
			for (i = 0; i < ID_DATA_PCS; i++)
			{
				if (ID_Receiver_DATA[i] == DATA_Packet_ID)
				{
					INquiry = i;
					i = ID_DATA_PCS;
					FLAG_IDCheck_OK = 1;
					if(Radio_Date_Type_bak==1){DATA_Packet_Control = DATA_Packet_Contro_buf;ID_SCX1801_DATA=DATA_Packet_ID;}
					else if(Radio_Date_Type_bak==2){Struct_DATA_Packet_Contro=Struct_DATA_Packet_Contro_buf;ID_SCX1801_DATA=DATA_Packet_ID;}
				} //2015.3.24?0?1?0?7?0?3?0?3?0?2?0?5 Control?0?4?0?4��?0?2?0?2?0?0��?0?8?ID?0?2?0?3��?0?3�C?0?2?0?3?0?0?0?4?0?2?0?6?0?7?0?2?0?2?0?7?0?1?0?1?0?2��?0?7?6?0?0?2?0?6?0?5?0?3��?0?3��?0?6?0?5?0?1?0?5?0?7?0?4����
				if ((FLAG_ID_Erase_Login == 1) && (FLAG_ID_Erase_Login_PCS == 1))
				{
					i = ID_DATA_PCS;
					FLAG_IDCheck_OK = 0;
					DATA_Packet_Control = DATA_Packet_Contro_buf;
				} //��?0?7?0?5?0?2?0?7?0?2?0?2��?0?8?0?3?0?1?0?3ID?0?4?6?4?0?3?0?2?0?5?6?1
			}

#endif


}
void TIM3_init(void)
{ // 2015.3.11????
    TIM3_CCMR1 = TIM3_CCMR1 | 0x70;
    TIM3_CCER1 = TIM3_CCER1 | 0x03;  //TIME3_CH1
    TIM3_ARRH = 0x08;                //0x07D0 -->PWM=2K        0x0880 -->PWM=1.83K
    TIM3_ARRL = 0x84;
                                     //TIM2_IER = 0x01;						// ??????????
    TIM3_CCR1H = 0x04;               //50%
    TIM3_CCR1L = 0x42;
    TIM3_PSCR = 0x02;                // ?????=Fsystem/(2(PSC[2:0])????4MHz=16MHz/2/2
    //TIM3_EGR_bit.UG=1;
    //TIM2_CR1 = 0x01;					// ?????????????????
    TIM3_CR1 = TIM3_CR1 | 0x01;
    TIM3_BKR = 0x80;
}
void Tone_OFF(void)
{                     // ???Tone   2015.3.11????
    TIM3_CR1_CEN = 0; // Timer 3 Disable
    TIM3_CCMR1 =  0x00;
    TIM3_CCER1 =  0x00; //????PWM?????????????????I/O
    PIN_BEEP = 0;
}
/*
   time_beepON、time_beepOFF单位时间�?0.4333333ms
*/
void BEEP_Module(UINT16 time_beepON, UINT16 time_beepOFF)
{
	UINT16 i;

	for (i = 0; i < time_beepON; i++)
	{
		//Receiver_Buzzer=!Receiver_Buzzer;   //蜂鸣器�?��??.08KHZ
		if (FG_beep_on == 0)
		{
			FG_beep_on = 1;
			FG_beep_off = 0;
			//BEEP_CSR2_BEEPEN = 1;
            TIM3_init();
        }
		Delayus(250); //80us
		Delayus(250); //80us
		Delayus(250); //80us
		ClearWDT();   // Service the WDT
	}
	for (i = 0; i < time_beepOFF; i++)
	{
		// Receiver_Buzzer=0;	//蜂鸣器�?��??.08KHZ
		if (FG_beep_off == 0)
		{
			FG_beep_off = 1;
			FG_beep_on = 0;
			//BEEP_CSR2_BEEPEN = 0;
            Tone_OFF();
        }
		//Delayus(240);
		Delayus(250); //80us
		Delayus(250); //80us
		Delayus(250); //80us
		ClearWDT();   // Service the WDT
	}

}

void BEEP_and_LED(void)
{
    Receiver_LED_OUT = 1;
    BEEP_Module(2300,1);
    FG_beep_on = 0;
    //BEEP_CSR2_BEEPEN = 0;
    TIME_Receiver_LED_OUT = 60; //185;
}

void Receiver_BEEP(void)
{
    UINT16 j;
    if (FLAG_Receiver_BEEP == 0)
    {
        FLAG_Receiver_BEEP = 1;
        for (j = 0; j < 3; j++)
          BEEP_Module(1050,1050);
    }
}

void TEST_beep(void)
{
	if(FLAG_testBEEP==1)
		BEEP_Module(300,1);
	else if(FLAG_testBEEP==2)
		{
		BEEP_Module(300,900);
		BEEP_Module(300,1);
		}
	else if(FLAG_testBEEP==3) BEEP_CSR2_BEEPEN = 1;
	FLAG_testBEEP=0;
}

void ID_Decode_OUT(void)
{
    u8 Control_i;

    Control_i = DATA_Packet_Control &  0xFF;
    if (TIMER1s)
    {
        if((FLAG_testNo91==1)||(FLAG_testNo91==2))
        {
            if((FLAG_testNo91SendUart==0)&&(FLAG_testNo91==1))
            {
                switch (Control_i)
                {
                    case 0x08: //open
//				            if(FLAG_testNo91_step==1)
//				            	{
                        Receiver_LED_OUT = 1;
                        ACKBack[2]=0xA1;
                        Send_Data(ACKBack, 3);
                        FLAG_testNo91SendUart=1;
//				            	}
                    break;
                    case 0x04: //stop
//				            if(FLAG_testNo91_step==2)
//				            	{
                        Receiver_LED_OUT = 1;
                        ACKBack[2]=0xA2;
                        Send_Data(ACKBack, 3);
                        FLAG_testNo91SendUart=1;
//				            	}
                    break;
                    case 0x02: //close
//				            if(FLAG_testNo91_step==3)
//				            	{
                        Receiver_LED_OUT = 1;
                        ACKBack[2]=0xA4;
                        Send_Data(ACKBack, 3);
                        FLAG_testNo91SendUart=1;
//				            	}
                    break;
                    default:
                        break;
                }

            }
        }
        else
        {
            switch (Control_i)
            {
                case 0x00:
                    APP429M_Tx_State();
                    break;
                case 0x14: //stop+login
                    Receiver_LED_OUT = 1;
                    TIMER250ms_STOP = 250;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_STOP = FG_allow_out;
                    if (TIMER1s < 3550)
                    {
                        Receiver_OUT_OPEN = FG_allow_out;
                        Receiver_OUT_CLOSE = FG_allow_out;
                        Receiver_BEEP();
                    }
                break;
                case 0x40: //�?动送信
                    if ((FG_auto_out == 0) && (Manual_override_TIMER == 0)&&(Radio_Date_Type_bak==1))
                    {
                        Receiver_LED_OUT = 1;
                        TIMER250ms_STOP = 0;
                        Receiver_OUT_VENT = FG_NOT_allow_out;
                        Receiver_OUT_CLOSE = FG_NOT_allow_out;
                        if (TIMER1s > 2000)
                        {
                            Receiver_OUT_STOP = FG_allow_out;
                            Receiver_OUT_OPEN = FG_NOT_allow_out;
                        } //1830
                        else if (TIMER1s > 1000)
                        {
                            Receiver_OUT_STOP = FG_NOT_allow_out;
                            Receiver_OUT_OPEN = FG_NOT_allow_out;
                        } //810
                        else
                        {
                            FG_auto_open_time = 1;
                            Receiver_OUT_STOP = FG_NOT_allow_out;
                            Receiver_OUT_OPEN = FG_allow_out;
                        }
                    }
                break;
                case 0x01: //VENT
                    Receiver_LED_OUT = 1;
                    if (Receiver_vent == 0)
                    { //受信机换气联动OFF
                        Receiver_OUT_STOP = FG_NOT_allow_out;
                        Receiver_OUT_VENT = FG_NOT_allow_out;
                        Receiver_OUT_OPEN = FG_allow_out;
                        Receiver_OUT_CLOSE = FG_allow_out;
                    }
                    else
                    { //受信机换气联动ON
                        Receiver_OUT_STOP = FG_NOT_allow_out;
                        Receiver_OUT_OPEN = FG_NOT_allow_out;
                        Receiver_OUT_CLOSE = FG_NOT_allow_out;
                        Receiver_OUT_VENT = FG_allow_out;
                    }
                break;
                case 0x02: //close
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_OPEN = FG_NOT_allow_out;
                    Receiver_OUT_STOP = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_CLOSE = FG_allow_out;
                    APP429M_Tx_State();
                break;
                case 0x04: //stop
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_OPEN = FG_NOT_allow_out;
                    Receiver_OUT_CLOSE = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_STOP = FG_allow_out;
                    APP429M_Tx_State();
                break;
                case 0x08: //open
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_STOP = FG_NOT_allow_out;
                    Receiver_OUT_CLOSE = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_OPEN = FG_allow_out;
                    APP429M_Tx_State();
                break;
                case 0x0C: //open+stop
                    Receiver_LED_OUT = 1;
                    TIMER250ms_STOP = 250;
                    Receiver_OUT_CLOSE = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_STOP = FG_allow_out;
                    if (FG_OUT_OPEN_CLOSE == 0)
                    {
                        FG_OUT_OPEN_CLOSE = 1;
                        TIME_OUT_OPEN_CLOSE = 25;
                    }
                    if (TIME_OUT_OPEN_CLOSE == 0)
                        Receiver_OUT_OPEN = FG_allow_out;
                break;
                case 0x06: //close+stop
                    Receiver_LED_OUT = 1;
                    TIMER250ms_STOP = 250;
                    Receiver_OUT_OPEN = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_STOP = FG_allow_out;
                    if (FG_OUT_OPEN_CLOSE == 0)
                    {
                        FG_OUT_OPEN_CLOSE = 1;
                        TIME_OUT_OPEN_CLOSE = 25;
                    }
                    if (TIME_OUT_OPEN_CLOSE == 0)
                        Receiver_OUT_CLOSE = FG_allow_out;
                break;
                case 0x0A: //close+OPEN
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_STOP = FG_NOT_allow_out;
                    Receiver_OUT_VENT = FG_NOT_allow_out;
                    Receiver_OUT_OPEN = FG_allow_out;
                    Receiver_OUT_CLOSE = FG_allow_out;
                break;
                case 0x09: //vent+OPEN
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_STOP = FG_NOT_allow_out;
                    Receiver_OUT_CLOSE = FG_NOT_allow_out;
                    Receiver_OUT_OPEN = FG_allow_out;
                    Receiver_OUT_VENT = FG_allow_out;
                break;
                case 0x03: //vent+close
                    Receiver_LED_OUT = 1;
                    Receiver_OUT_STOP = FG_NOT_allow_out;
                    Receiver_OUT_OPEN = FG_NOT_allow_out;
                    Receiver_OUT_CLOSE = FG_allow_out;
                    Receiver_OUT_VENT = FG_allow_out;
                break;
                default:
                    break;
            }
            if(Radio_Date_Type_bak==2)
            {             //429M   半开信号/半闭
                if(((DATA_Packet_Control&0xDF)>0x80)&&((DATA_Packet_Control&0x20)==0x00))
                {
                    if((DATA_Packet_Control&0xDF)<0xC0){
                        FLAG__Semi_open_T=1;
                        FLAG__Semi_close_T=0;
                        Receiver_LED_OUT=1;
                        Receiver_OUT_STOP=FG_NOT_allow_out;
                        Receiver_OUT_CLOSE=FG_NOT_allow_out;
                        Receiver_OUT_OPEN=FG_allow_out;
                        TIMER250ms_STOP=((TIMER_Semi_open+1)*1000/107)*100;
                    }
                    else
                    {
                        FLAG__Semi_open_T=0;
                        FLAG__Semi_close_T=1;
                        Receiver_LED_OUT=1;
                        Receiver_OUT_STOP=FG_NOT_allow_out;
                        Receiver_OUT_CLOSE=FG_allow_out;
                        Receiver_OUT_OPEN=FG_NOT_allow_out;
                        TIMER250ms_STOP=((TIMER_Semi_close+1)*1000/107)*100;
                    }
                }
                if((DATA_Packet_Control==0x7F)&&(Flag_ERROR_Read==0)&&(Flag_shutter_stopping==0))
                {
                    Flag_ERROR_Read=1;
                    FLAG_APP_TX_fromUART_err_read=0;
                    Send_Data(Send_err_com, 7);
                    Flag_ERROR_Read_once_again=1;
                    TIME_ERROR_Read_once_again=17;
                    Time_error_read_timeout=100;
                }
            }
            if((FLAG__Semi_open_T==1)||(FLAG__Semi_close_T==1))
            {
                if((DATA_Packet_Control==0x02)||(DATA_Packet_Control==0x04)||(DATA_Packet_Control==0x08)||(DATA_Packet_Control==0x01)||(DATA_Packet_Control==0x20)||(DATA_Packet_Control==0x40)
                    ||(DATA_Packet_Control==0x9)||(DATA_Packet_Control==0x03)||(DATA_Packet_Control==0x0C)||(DATA_Packet_Control==0x06)||(DATA_Packet_Control==0x0A))
                {
                    //2015.12.29追加，在半开、半�?动作�?，�?�信机（�?+�?）信号，让停止继电器不动�?
                    FLAG__Semi_open_T=0;FLAG__Semi_close_T=0;TIMER250ms_STOP=0;
                }
            }

            if(((DATA_Packet_Control==0x00)||(DATA_Packet_Control==0x02)||(DATA_Packet_Control==0x04)||(DATA_Packet_Control==0x08)||(DATA_Packet_Control==0x01)
               ||(DATA_Packet_Control==0x20)||(DATA_Packet_Control==0x40)||((FLAG__Semi_open_T==1)||(FLAG__Semi_close_T==1)))&&(FLAG_APP_TX_fromOUT==0)&&(Radio_Date_Type_bak==2)&&(FLAG_APP_TX==0)&&(FLAG_APP_TX_once==1))
            {
                FLAG_APP_TX_fromOUT=1;
                if(DATA_Packet_Control==0x00)
                        TIME_APP_TX_fromOUT=35;//15+DEF_APP_TX_freq*8;  //350ms
                else    TIME_APP_TX_fromOUT=35;//15+DEF_APP_TX_freq*8;  //350ms
            }
        }

    }

    else
    {
        //           if(FLAG_APP_Reply==1){FLAG_APP_Reply=0;ID_data.IDL=DATA_Packet_ID;Control_code=HA_Status;FLAG_HA_START=1;}
        //           if(FLAG_426MHz_Reply==1){FLAG_426MHz_Reply=0;ID_data.IDL=DATA_Packet_ID;Control_code=HA_Status+4;FLAG_HA_START=1;}   //受信器自动发送HA状�?�码为实际HA�?4
        if ((FG_auto_out == 1) && (TIME_auto_out == 0))
        {
            FG_auto_out = 0;
            TIME_auto_close = 270;
            Receiver_LED_OUT = 1;
        } //300
        if (TIME_auto_close)
        {
            if (TIME_auto_close > 180)
            {
                Receiver_OUT_STOP = FG_allow_out;
                Receiver_OUT_CLOSE = FG_NOT_allow_out;
            } //200
            else if (TIME_auto_close > 90)
            {
                Receiver_OUT_STOP = FG_NOT_allow_out;
                Receiver_OUT_CLOSE = FG_NOT_allow_out;
            } //100
            else
            {
                Receiver_OUT_STOP = FG_NOT_allow_out;
                Receiver_OUT_CLOSE = FG_allow_out;
            }
        }
        else
        {
            FG_auto_manual_mode = 0;
            Receiver_OUT_CLOSE = FG_NOT_allow_out;
        }
        FG_First_auto = 0;
        FLAG_Receiver_BEEP = 0;
		Receiver_OUT_OPEN = FG_NOT_allow_out;
		Receiver_OUT_VENT = FG_NOT_allow_out;
        if ((FLAG_ID_Erase_Login == 1) || (FLAG_ID_Login == 1) || (TIME_auto_close)||(FLAG_ID_SCX1801_Login==1))
            ;
        else if (TIME_Receiver_LED_OUT > 0)
            Receiver_LED_OUT = 1;
        else if(TIME_power_led==0)
            Receiver_LED_OUT = 0;

        if((FLAG__Semi_open_T==1)||(FLAG__Semi_close_T==1))
        {
//                   if(HA_Status==0x83)TIMER250ms_STOP=0;     //2015.12.29追加，在半开、半�?动作�?，受信机的状态变成异�?1的时候，让停止继电器不动�?
            if((TIMER250ms_STOP<1000)&&(TIMER250ms_STOP>0))
            {
                Receiver_OUT_STOP=FG_allow_out;
                Receiver_LED_OUT=1;
            }
            else if(TIMER250ms_STOP==0)
            {
                Receiver_OUT_STOP=FG_NOT_allow_out;
                FLAG__Semi_open_T=0;
                FLAG__Semi_close_T=0;
            }
        }
        else if((TIMER250ms_STOP==0)&&(TIME_auto_close==0))
        {
            Receiver_OUT_STOP=FG_NOT_allow_out;
            FG_OUT_OPEN_CLOSE=0;
        }    //2015.3.23�?�?

        if (FG_auto_open_time == 1)
        {
            FG_First_auto = 0;
            FG_auto_out = 1;
            FG_auto_open_time = 0;
        }
        if ((TIMER250ms_STOP == 0) && (TIME_auto_close == 0))
        {
            Receiver_OUT_STOP = FG_NOT_allow_out;
            FG_OUT_OPEN_CLOSE = 0;
        }
        FLAG_testNo91SendUart=0;
    }
    if (TIMER300ms == 0)
        FG_Receiver_LED_RX = 0; //Receiver_LED_RX=0;
}

void Freq_Scanning(void)
{
	if (TIMER18ms == 0)
		{
			if (Flag_FREQ_Scan == 0)
/*
			if ((Flag_FREQ_Scan == 0)&&((FLAG_ID_Erase_Login == 1) || (FLAG_ID_Login == 1) ||(FLAG_ID_SCX1801_Login==1)||
										  ((FLAG_ID_Erase_Login==0)&&(FLAG_ID_Login==0)&&(FLAG_ID_SCX1801_Login==0)&&(PROFILE_CH_FREQ_32bit_200002EC != 426075000)))
			   )  //����ģʽʱ������426.075MHz���źţ�ֻ���ڵ�¼ģʽʱ�Ž��ܡ�
*/
			{
				if (ADF7030_Read_RESIGER(0x4000380C, 1, 0) != 0)
				{
		             FG_Receiver_LED_RX = 1;

					Flag_FREQ_Scan = 1;
					if(Radio_Date_Type==1)
					  {TIMER18ms = 82;TIMER300ms = 600; }
					else if(Radio_Date_Type==2)
					  {TIMER18ms = 130; TIMER300ms = 100;  }

					return;
				}
			}

			ADF7030_Change_Channel();
			ADF7030Init();	   //��Ƶ��ʼ��

			if(Radio_Date_Type==1)
			  TIMER18ms = 18;
			else if(Radio_Date_Type==2)
			  TIMER18ms = 18;

			Flag_FREQ_Scan = 0;
		}

}

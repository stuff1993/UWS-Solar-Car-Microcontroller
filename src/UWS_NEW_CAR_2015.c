/*
===============================================================================
 Name        : UWS_NEW_CAR_2015.c
 Author      : Stuart Gales
 Version     : 1.0
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdint.h>
#include <stdio.h>

#include "timer.h"
#include "lcd.h"
#include "pwm.h"
#include "adc.h"
#include "can.h"
#include "IAP.h"
#include "i2c.h"
#include "menu.h"
#include "dash.h"
#include "struct.h"

/////////////////////////////   CAN    ////////////////////////////////
CAN_MSG MsgBuf_TX1, MsgBuf_TX2; /* TX and RX Buffers for CAN message */
CAN_MSG MsgBuf_RX1, MsgBuf_RX2; /* TX and RX Buffers for CAN message */
volatile uint32_t CAN1RxDone = FALSE, CAN2RxDone = FALSE;
///////////////////////////////////////////////////////////////////////

/////////////////////////////   I2C    ////////////////////////////////
extern volatile uint8_t I2CMasterBuffer[I2C_PORT_NUM][BUFSIZE];
extern volatile uint32_t I2CWriteLength[I2C_PORT_NUM];
extern volatile uint32_t I2CReadLength[I2C_PORT_NUM];
extern volatile uint8_t I2CSlaveBuffer[I2C_PORT_NUM][BUFSIZE];
///////////////////////////////////////////////////////////////////////

volatile unsigned char SWITCH_IO	= 0;

uint16_t THR_POS = 0;
uint16_t RGN_POS = 0;
uint16_t MECH_BRAKE = 0;

uint16_t PWMBL 	= 0;

uint32_t ADC_A 	= 0;
uint32_t ADC_B 	= 0;
uint32_t ADC_C 	= 0;

/// MPPTs
MPPT MPPT1;
MPPT MPPT2;

/// Relayed MPPTs
fakeMPPTFRAME fakeMPPT1;
fakeMPPTFRAME fakeMPPT2;

/// Motor Controller
MOTORCONTROLLER ESC;

int main(void) {

	SystemInit();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 100);		// 10mS Systicker.

	ADCInit(ADC_CLK);
	setPORTS();

	setLCD();
	lcdCLEAR();
	I2C1Init();

	PWM_Init(1, 1300);		//1300 PWM steps. 1000/1300 gives roughly 5volts on the A-OUT Ports.
	PWM_Start(1);

	setCANBUS1();
	setCANBUS2();

	BOD_Init();

	displayINTRO();

	recallVariables();

	menuInit();

	while(1) {
		if(((ESC1.ERROR & 0x2) || (ESC2.ERROR & 0x2)) && !STATS.SWOC_ACK) // on unacknowledged SWOC error, show error screen
		{
			MENU.errors[0]();
		}
		else if (((ESC1.ERROR & 0x1) || (ESC2.ERROR & 0x1)) && !STATS.HWOC_ACK) // on unacknowledged SWOC error, show error screen
		{
			MENU.errors[1]();
		}
		else
		{
			if(STATS.SWOC_ACK && !((ESC1.ERROR & 0x2) || (ESC2.ERROR & 0x2))) // if acknowledged previous error is reset
			{STATS.SWOC_ACK = FALSE;}
			if(STATS.HWOC_ACK && !((ESC1.ERROR & 0x1) || (ESC2.ERROR & 0x1))) // if acknowledged previous error is reset
			{STATS.HWOC_ACK = FALSE;}

			MENU.menus[MENU_POS]();
		}

		pollMPPTCAN();
		checkBUTTONS();
		driveROUTINE();
		checkMPPTCOMMS();
		tx500CAN();
		doCALCULATIONS();
    }
    return 0 ;
}

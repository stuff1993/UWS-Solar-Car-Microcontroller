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

#include "inttofloat.h"
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
uint8_t MECH_BRAKE = 0;

uint16_t PWMBL 	= 0;

/// MPPTs
MPPT MPPT1;
MPPT MPPT2;

/// Relayed MPPTs
fakeMPPTFRAME fakeMPPT1;
fakeMPPTFRAME fakeMPPT2;

/// Motor Controller
MOTORCONTROLLER ESC;

/******************************************************************************
** Function name:		BOD_IRQHandler
**
** Description:			Brown-out detection handler
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void BOD_IRQHandler (void)
{
	DUTYBL = 0;
	LPC_PWM1->LER = LER1_EN | LER2_EN | LER3_EN | LER6_EN;

	//writeDATA(ADDRESS1, PWMBL);					// writes the BL to flash location
	REVERSE_ON;NEUTRAL_ON;REGEN_ON;DRIVE_ON;	// write complete
}

/******************************************************************************
** Function name:		SysTick_Handler
**
** Description:			System clock event handler. Fires every 10mS
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void SysTick_Handler (void)
{
	CLOCK.T_mS++;

	if (CLOCK.T_mS % 10 == 0) // Every 100 mS
	{
		// TODO: Transmit drive CAN packet
	}

	////////////////////////  AMP/hr and WATT/hr Calculations  ///////////////////////
	ESC.WattHrs = ESC.WattHrs + (ESC.Watts/360000);

	MPPT1.WattHrs = MPPT1.WattHrs + (MPPT1.Watts/360000);
	MPPT2.WattHrs = MPPT2.WattHrs + (MPPT2.Watts/360000);

	BMU.WattHrs = BMU.WattHrs + BMU.Watts/360000;

	////////////////////////  Odometer Calculations  ///////////////////////
	if(ESC.Velocity_KMH>0){STATS.ODOMETER 	  = STATS.ODOMETER	   + ESC.Velocity_KMH/360000;}

	if(ESC.Velocity_KMH<0){STATS.ODOMETER_REV = STATS.ODOMETER_REV + ESC.Velocity_KMH/360000;}
	//////////////////////////////////////////////////////////////////////////////////

	if(CLOCK.T_mS >= 100) // Calculate time
	{
		CLOCK.T_mS = 0;CLOCK.T_S++;

		if(MPPT1.Connected>0){MPPT1.Connected--;} // if disconnected for 2 seconds. Then FLAG disconnect.
		if(MPPT2.Connected>0){MPPT2.Connected--;} // if disconnected for 2 seconds. Then FLAG disconnect.

		storeVariables(); // Store data in eeprom every second

		if(CLOCK.T_S >= 60){CLOCK.T_S = 0;CLOCK.T_M++;
		if(CLOCK.T_M >= 60){CLOCK.T_M = 0;CLOCK.T_H++;
		if(CLOCK.T_H >= 24){CLOCK.T_H = 0;CLOCK.T_D++;}}}
	}
}

/******************************************************************************
** Function name:		pollMPPTCAN
**
** Description:			1. Sends request packet to MPPT (125K CAN Bus)
** 						2. Sends previous MPPT packet to car (500K CAN Bus)
** 						3. Receives new packet and extracts data (125K CAN Bus)
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void pollMPPTCAN (void)
{
	STATS.MPPT_POLL_COUNT ^= 0b1; 								// Toggle bit. Selects which MPPT to poll this round

	if((LPC_CAN2->GSR & (1 << 3)) && STATS.MPPT_POLL_COUNT)		// If previous transmission is complete, send message;
	{
		MsgBuf_TX2.MsgID = MPPT2_BASE_ID; 						// Explicit Standard ID
		CAN2_SendMessage( &MsgBuf_TX2 );
	}

	else if(LPC_CAN2->GSR & (1 << 3))							// If previous transmission is complete, send message;
	{
		MsgBuf_TX2.MsgID = MPPT1_BASE_ID; 						// Explicit Standard ID
		CAN2_SendMessage( &MsgBuf_TX2 );
	}


	if((LPC_CAN1->GSR & (1 << 3)) && STATS.MPPT_POLL_COUNT)		// If previous transmission is complete, send message;
	{
		MsgBuf_TX1.Frame = 0x00080000; 							// 11-bit, no RTR, DLC is 8 bytes
		MsgBuf_TX1.MsgID = DASH_RPLY_2; 						// Explicit Standard ID
		if (MPPT2.Connected) // Relay data
		{
			MsgBuf_TX1.DataA = fakeMPPT2.DataA;
			MsgBuf_TX1.DataB = fakeMPPT2.DataB;
		}
		else // No data
		{
			MsgBuf_TX1.DataA = 0x0;
			MsgBuf_TX1.DataB = 0x0;
		}
		CAN1_SendMessage( &MsgBuf_TX1 );
	}
	else if(LPC_CAN1->GSR & (1 << 3))							// If previous transmission is complete, send message;
	{
		MsgBuf_TX1.Frame = 0x00080000; 							// 11-bit, no RTR, DLC is 8 bytes
		MsgBuf_TX1.MsgID = DASH_RPLY_1; 						// Explicit Standard ID
		if (MPPT1.Connected) // Relay data
		{
			MsgBuf_TX1.DataA = fakeMPPT1.DataA;
			MsgBuf_TX1.DataB = fakeMPPT1.DataB;
		}
		else // No data
		{
			MsgBuf_TX1.DataA = 0x0;
			MsgBuf_TX1.DataB = 0x0;
		}
		CAN1_SendMessage( &MsgBuf_TX1 );
	}


	/* please note: FULLCAN identifier will NOT be received as it's not set
		in the acceptance filter. */
	if ( CAN2RxDone == TRUE )
	{
		CAN2RxDone = FALSE;

		if		(MsgBuf_RX2.MsgID == MPPT1_REPLY){extractMPPTDATA(&MPPT1, &fakeMPPT1);}
		else if	(MsgBuf_RX2.MsgID == MPPT2_REPLY){extractMPPTDATA(&MPPT2, &fakeMPPT2);}


		// Everything is correct, reset buffer
		MsgBuf_RX2.Frame = 0x0;
		MsgBuf_RX2.MsgID = 0x0;
		MsgBuf_RX2.DataA = 0x0;
		MsgBuf_RX2.DataB = 0x0;
	} // Message on CAN 2 received
}

/******************************************************************************
** Function name:		extractMPPTDATA
**
** Description:			Extracts data from CAN 2 Receive buffer into MPPT structure.
**
** Parameters:			1. Address of MPPT to extract to
** 						2. Address of fakeMPPT to use for retransmit
** Returned value:		None
**
******************************************************************************/
void extractMPPTDATA (MPPT *_MPPT, fakeMPPTFRAME *_fkMPPT)
{
	uint32_t _VIn = 0;
	uint32_t _IIn = 0;
	uint32_t _VOut = 0;

	uint32_t _Data_A = MsgBuf_RX2.DataA;
	uint32_t _Data_B = MsgBuf_RX2.DataB;

	_fkMPPT->DataA = _Data_A;
	_fkMPPT->DataB = _Data_B;

	// Status Flags
	_MPPT->BVLR = ((_Data_A & 0x80) >> 7);  	// Battery voltage level reached
	_MPPT->OVT  = ((_Data_A & 0x40) >> 6);  	// Over temperature
	_MPPT->NOC  = ((_Data_A & 0x20) >> 5);  	// No Connection
	_MPPT->UNDV = ((_Data_A & 0x10) >> 4);  	// Under voltage on input

	// Power Variables
	_VIn = ((_Data_A & 0b11) << 8);  			// Masking and shifting the upper 2 MSB
	_VIn = _VIn + ((_Data_A & 0xFF00) >> 8); 	// Masking and shifting the lower 8 LSB
	_VIn = _VIn * 1.50;							// Scaling

	_Data_A = (_Data_A >> 16);
	_IIn = ((_Data_A & 0b11) << 8); 			// Masking and shifting the lower 8 LSB
	_IIn = _IIn + ((_Data_A & 0xFF00) >> 8);	// Masking and shifting the upper 2 MSB
	_IIn = _IIn * 0.87;  						// Scaling

	_VOut = ((_Data_B & 0b11) << 8);  			// Masking and shifting the upper 2 MSB
	_VOut = _VOut + ((_Data_B & 0xFF00) >> 8); 	// Masking and shifting the lower 8 LSB
	_VOut = _VOut * 2.10;						// Scaling

	// TODO: May run iir filter
	_MPPT->Tmp = (((_Data_B & 0xFF0000) >> 16) + 7 * _MPPT->Tmp)/8;

	// Update the global variables after IIR filtering
	_MPPT->VIn = iirFILTER(_VIn, _MPPT->VIn);	//infinite impulse response filtering
	_MPPT->IIn = iirFILTER(_IIn, _MPPT->IIn);
	_MPPT->VOut = iirFILTER(_VOut, _MPPT->VOut);
	if(_MPPT->Connected<2){_MPPT->Connected = 2;}
}

/******************************************************************************
** Function name:		checkBUTTONS
**
** Description:			Checks majority of inputs (Switches, Left, Right)
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void checkBUTTONS (void)
{
	unsigned char OLD_IO = SWITCH_IO;

	SWITCH_IO = 0;
	SWITCH_IO |= (FORWARD << 0);
	SWITCH_IO |= (REVERSE << 1);
	SWITCH_IO |= (SPORT_MODE << 2);

	if(OLD_IO != SWITCH_IO){buzzer(50);}	// BEEP if toggle position has changed.

	if(RIGHT)
	{
		MENU.MENU_POS++;delayMs(1, 400);buzzer(50);
		if(MENU.MENU_POS>MENU_ITEMS){MENU.MENU_POS = 0;}
		if(MENU.MENU_POS==1){buzzer(300);}
		if((ESC.ERROR & 0x2) && !STATS.SWOC_ACK){STATS.SWOC_ACK = TRUE;}
		if((ESC.ERROR & 0x1) && !STATS.HWOC_ACK){STATS.HWOC_ACK = TRUE;BUZZER_OFF}

		lcdCLEAR();
		MENU.SELECTED = 0;
		MENU.ITEM_SELECTOR = 0;
		MENU.SUBMENU_POS = 0;
	}

	if(LEFT)
	{
		MENU.MENU_POS--;delayMs(1, 400);buzzer(50);
		if(MENU.MENU_POS<0){MENU.MENU_POS = MENU_ITEMS - 1;}
		if(MENU.MENU_POS==1){buzzer(300);}
		if((ESC.ERROR & 0x2) && !STATS.SWOC_ACK){STATS.SWOC_ACK = TRUE;}
		if((ESC.ERROR & 0x1) && !STATS.HWOC_ACK){STATS.HWOC_ACK = TRUE;BUZZER_OFF}

		lcdCLEAR();
		MENU.SELECTED = 0;
		MENU.ITEM_SELECTOR = 0;
		MENU.SUBMENU_POS = 0;
	}

	// Check Sports/Economy switch
	if(SWITCH_IO & 0x4)	{STATS.DRIVE_MODE = SPORTS;STATS.RAMP_SPEED = SPORTS_RAMP_SPEED;}
	else				{STATS.DRIVE_MODE = ECONOMY;STATS.RAMP_SPEED = ECONOMY_RAMP_SPEED;}
}

/******************************************************************************
** Function name:		driveROUTINE
**
** Description:			Reads drive inputs and
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void driveROUTINE (void)
{
	uint32_t ADC_A;
	uint32_t ADC_B;
	uint32_t ADC_C;

	////////////// THROTTLE ////////////////
	if(!CRUISE.ACTIVE)
	{
		ADC_A = (ADCRead(2) + ADCRead(2) + ADCRead(2) + ADCRead(2) + ADCRead(2) + ADCRead(2) + ADCRead(2) + ADCRead(2))/8;

		THR_POS = (1500 - ADC_A);
		THR_POS = (THR_POS * 9)/10;
		if(THR_POS < 0){THR_POS = 0;}
		if(THR_POS > 1000){THR_POS = 1000;}
	}

	////////////// REGEN ////////////////
	ADC_B = (ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1))/8;

	// Calibrate endpoints and add deadband.
	RGN_POS = (ADC_B - 1660);
	RGN_POS = (RGN_POS * 9)/10;
	if(RGN_POS < 0){RGN_POS = 0;}
	if(RGN_POS > 1000){RGN_POS = 1000;}
	if(RGN_POS){STATS.CR_ACT = OFF;}

	////////////// MECH BRAKE ////////////////
	ADC_C = (ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0))/8;

	if(ADC_C > 2000){MECH_BRAKE = TRUE;STATS.CR_ACT = OFF;}
	else{MECH_BRAKE = FALSE;}


	////////////// DRIVE LOGIC ////////////////
	if (!MECH_BRAKE && (FORWARD || REVERSE)){
		if(!STATS.CR_ACT && FORWARD && !RGN_POS && ((DRIVE.Current * 1000) < THR_POS))		{DRIVE.Speed_RPM = 30000; DRIVE.Current += (STATS.RAMP_SPEED / 1000.0);}
		else if(!STATS.CR_ACT && FORWARD && !RGN_POS)										{DRIVE.Speed_RPM = 30000; DRIVE.Current = (THR_POS / 1000.0);}
		else if(!STATS.CR_ACT && REVERSE && !RGN_POS && ((DRIVE.Current * 1000) < THR_POS))	{DRIVE.Speed_RPM = -30000; DRIVE.Current += (STATS.RAMP_SPEED / 1000.0);}
		else if(!STATS.CR_ACT && REVERSE && !RGN_POS)										{DRIVE.Speed_RPM = -30000; DRIVE.Current = (THR_POS / 1000.0);}
		else if(!STATS.CR_ACT && RGN_POS && ((DRIVE.Current * 1000) < RGN_POS))				{DRIVE.Speed_RPM = 0; DRIVE.Current += (REGEN_RAMP_SPEED / 1000.0);}
		else if(!STATS.CR_ACT && RGN_POS)													{DRIVE.Speed_RPM = 0; DRIVE.Current = (RGN_POS / 2);}
		else if(STATS.CR_ACT){DRIVE.Current = 1.0; DRIVE.Speed_RPM = STATS.CRUISE_SPEED / ((60 * 3.14 * WH_RADIUS_M) / 1000.0);}
		else{DRIVE.Speed_RPM = 0; DRIVE.Current = 0;}}
	else{DRIVE.Speed_RPM = 0; DRIVE.Current = 0;}

	/*
	if(!MECH_BRAKE && (FORWARD || REVERSE)){
		if(!CRUISE_CHECK && !RGN_POS && PWMA < THR_POS){PWMA += STATS.RAMP_SPEED;PWMC = 0;}
		else if(!CRUISE_CHECK && !RGN_POS){PWMA = THR_POS;PWMC = 0;}
		else if(!CRUISE_CHECK && RGN_POS && PWMC < RGN_POS){PWMC += REGEN_RAMP_SPEED;PWMA = 0;}
		else if(!CRUISE_CHECK && RGN_POS){PWMC = RGN_POS/2;PWMA = 0;}
		else if(CRUISE_CHECK && (CRUISE.CO > 1000) && (PWMA < (CRUISE.CO - 1000))){PWMA += STATS.RAMP_SPEED;PWMC = 0;}
		else if(CRUISE_CHECK && (CRUISE.CO > 1000)){PWMA = (CRUISE.CO - 1000);PWMC = 0;}
		else if(CRUISE_CHECK && (CRUISE.CO < 1000) && (PWMC < (1000 - CRUISE.CO))){PWMC += REGEN_RAMP_SPEED;PWMA = 0;}
		else if(CRUISE_CHECK && (CRUISE.CO < 1000)){PWMC = ((1000 - CRUISE.CO) / 2);PWMA = 0;}
		else if(CRUISE_CHECK && CRUISE.CO == 1000){PWMA = 0; PWMC = 0;}
		else{PWMA = 0;PWMB = 0;PWMC = 0;}}
	else{PWMA = 0;PWMB = 0;PWMC = 0;}
	 */

	////////////// LIGHTS ////////////////
	if(MECH_BRAKE || RGN_POS){BRAKELIGHT_ON;}
	else{BRAKELIGHT_OFF;}

	if(!RGN_POS)
	{
		if(REVERSE)
		{REVERSE_ON;NEUTRAL_OFF;REGEN_OFF;DRIVE_OFF;}

		else if(FORWARD)
		{REVERSE_OFF;NEUTRAL_OFF;REGEN_OFF;DRIVE_ON;}

		else
		{REVERSE_OFF;NEUTRAL_ON;REGEN_OFF;DRIVE_OFF;}
	}
	else
	{
		if(REVERSE)
		{REVERSE_ON;NEUTRAL_OFF;REGEN_ON;DRIVE_OFF;}

		else if(FORWARD)
		{REVERSE_OFF;NEUTRAL_OFF;REGEN_ON;DRIVE_ON;}

		else
		{REVERSE_OFF;NEUTRAL_ON;REGEN_OFF;DRIVE_OFF;}
	}

	////////////// UPDATE OUTPUTS ////////////////
	DUTYBL = PWMBL;
	// reset DAC registers.
	LPC_PWM1->LER = LER6_EN;


	if(ESC.Velocity_KMH > STATS.MAX_SPEED)
	{
		STATS.MAX_SPEED = ESC.Velocity_KMH;
	}
}

/******************************************************************************
** Function name:		checkMPPTCOMMS
**
** Description:			Resets MPPTs on disconnect
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void checkMPPTCOMMS (void)
{
	if(!MPPT1.Connected)
	{
		// Reset Power Variables if not connected
		MPPT1.VIn = 0;
		MPPT1.IIn = 0;
		MPPT1.VOut = 0;
		MPPT1.Watts = 0;

		fakeMPPT1.DataA = 0;
		fakeMPPT1.DataB = 0;
	}

	if(!MPPT2.Connected)
	{
		// Reset Power Variables if not connected
		MPPT2.VIn = 0;
		MPPT2.IIn = 0;
		MPPT2.VOut = 0;
		MPPT2.Watts = 0;

		fakeMPPT2.DataA = 0;
		fakeMPPT2.DataB = 0;
	}
}

/******************************************************************************
** Function name:		tx500CAN
**
** Description:			Sends requested packets on 500K CAN Bus
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void tx500CAN (void) // TODO: Rewrite
{
	/* please note: FULLCAN identifier will NOT be received as it's not set
	in the acceptance filter. */
	if ( CAN1RxDone == TRUE )
	{
		CAN1RxDone = FALSE;

		if((MsgBuf_RX1.MsgID == DASH_RQST) && (MsgBuf_RX1.DataA == 0x4C4C494B) && (MsgBuf_RX1.DataB == 0x45565244)) // Data = KILLDRVE
		{

			lcdCLEAR();
			char buffer[20];
			/*
			DUTYA = 0;
			DUTYB = 0;
			DUTYC = 0;
			*/

			sprintf(buffer, "--   KILL DRIVE   --");
			lcd_putstring(0,0, buffer);
			lcd_putstring(1,0, buffer);
			lcd_putstring(2,0, buffer);
			lcd_putstring(3,0, buffer);

			while(FORWARD || REVERSE)
			{
				buzzer(1000);
				delayMs(1,500);
			}

			lcdCLEAR();
		}

		////////////////////////////////////////////////////////////////////////////////////

		if((LPC_CAN1->GSR & (1 << 3))  && !STATS.MPPT_POLL_COUNT)		// If previous transmission is complete, send message;
		{
			MsgBuf_TX1.Frame = 0x00080000; 							/* 11-bit, no RTR, DLC is 8 bytes */
			MsgBuf_TX1.MsgID = DASH_RPLY_3; 						/* Explicit Standard ID */
			MsgBuf_TX1.DataA = convertToUint(MPPT1.WattHrs + MPPT2.WattHrs);		// MPPT WHR TOTAL
			MsgBuf_TX1.DataB = convertToUint(BMU.WattHrs);		// BMU WHR TOTAL
			CAN1_SendMessage( &MsgBuf_TX1 );
		}

		else if((LPC_CAN1->GSR & (1 << 3)))							// If previous transmission is complete, send message;
		{
			MsgBuf_TX1.Frame = 0x00080000; 							/* 11-bit, no RTR, DLC is 8 bytes */
			MsgBuf_TX1.MsgID = DASH_RPLY_4; 						/* Explicit Standard ID */
			MsgBuf_TX1.DataA = convertToUint(MPPT1.Watts + MPPT2.Watts);	// MPPT CURRENT WATTS
			MsgBuf_TX1.DataB = convertToUint(BMU.Watts);					// BMU CURRENT WATTS
			CAN1_SendMessage( &MsgBuf_TX1 );
		}


		// Clear Rx Buffer
		MsgBuf_RX1.Frame = 0x0;
		MsgBuf_RX1.MsgID = 0x0;
		MsgBuf_RX1.DataA = 0x0;
		MsgBuf_RX1.DataB = 0x0;
	}
}

/******************************************************************************
** Function name:		doCALCULATIONS
**
** Description:			Calculates instantaneous values and peaks
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void doCALCULATIONS (void)
{
	// Calculate Watts been used by the motor controller
	ESC.Watts = (ESC.Bus_V * ESC.Bus_I);

	// Calculate array Watts
	MPPT1.Watts = (MPPT1.VIn * MPPT1.IIn);
	MPPT2.Watts = (MPPT2.VIn * MPPT2.IIn);
	MPPT1.Watts = MPPT1.Watts/1000;
	MPPT2.Watts = MPPT2.Watts/1000;

	// Check peaks
	if(ESC.Watts > ESC.MAX_Watts){ESC.MAX_Watts = ESC.Watts;}

	if(ESC.Bus_I > ESC.MAX_Bus_I){ESC.MAX_Bus_I = ESC.Bus_I;}
	if(ESC.Bus_V > ESC.MAX_Bus_V){ESC.MAX_Bus_V = ESC.Bus_V;}

	if(MPPT1.Watts > MPPT1.MAX_Watts){MPPT1.MAX_Watts = MPPT1.Watts;}
	if(MPPT2.Watts > MPPT2.MAX_Watts){MPPT2.MAX_Watts = MPPT2.Watts;}


	// TODO: Move to CAN Receive
	float tempcurr = (float)BMU1.BATTERY_CURRENT;
	BMU1.BUS_CURRENT = ((tempcurr/1000) + 15*BMU1.BUS_CURRENT)/16;
	BMU1.WATTS = BMU1.BUS_CURRENT*ESC1.BUS_VOLTAGE;
}

/******************************************************************************
** Function name:		main
**
** Description:			Program entry point. Contains initializations and menu loop
**
** Parameters:			None
** Returned value:		Program exit value
**
******************************************************************************/
int main (void)
{

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

	while(1) { // Exiting this loop ends the program
		if((ESC.ERROR & 0x2) && !STATS.SWOC_ACK) // on unacknowledged SWOC error, show error screen
		{
			MENU.errors[0]();
		}
		else if ((ESC.ERROR & 0x1) && !STATS.HWOC_ACK) // on unacknowledged SWOC error, show error screen
		{
			MENU.errors[1]();
		}
		else
		{
			if(STATS.SWOC_ACK && !(ESC.ERROR & 0x2)) // if acknowledged previous error is reset
			{STATS.SWOC_ACK = FALSE;}
			if(STATS.HWOC_ACK && !(ESC.ERROR & 0x1)) // if acknowledged previous error is reset
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

    return 0; // For compilers sanity
}

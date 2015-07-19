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

#include "type.h"
#include "inttofloat.h"
#include "timer.h"
#include "lcd.h"
#include "pwm.h"
#include "adc.h"
#include "can.h"
#include "i2c.h"
#include "struct.h"
#include "dash.h"
#include "menu.h"

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

	REVERSE_ON;NEUTRAL_ON;REGEN_ON;DRIVE_ON;FAULT_ON;ECO_ON;SPORTS_ON;
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

	// MinorSec: DIU CAN Heartbeat
	if (CLOCK.T_mS % 10 == 0) // Every 100 mS send heartbeat CAN packets
	{
		MsgBuf_TX1.Frame = 0x00080000;
		MsgBuf_TX1.MsgID = ESC_BASE + 1;
		MsgBuf_TX1.DataA = conv_float_uint(DRIVE.Speed_RPM);
		MsgBuf_TX1.DataB = conv_float_uint(DRIVE.Current);
		CAN1_SendMessage( &MsgBuf_TX1 );

		MsgBuf_TX1.Frame = 0x00080000;
		MsgBuf_TX1.MsgID = ESC_BASE + 2;
		MsgBuf_TX1.DataA = conv_float_uint(1);
		MsgBuf_TX1.DataB = 0x0;
		CAN1_SendMessage( &MsgBuf_TX1 );

		// TODO: Check if required
		/*
		MsgBuf_TX1.Frame = 0x00080000;
		MsgBuf_TX1.MsgID = ESC_BASE + 5;
		MsgBuf_TX1.DataA = STATS.IGNITION;
		MsgBuf_TX1.DataB = 0x0;
		CAN1_SendMessage( &MsgBuf_TX1 );
		*/
	}

	// MinorSec:  Time sensitive Calculations
	ESC.WattHrs += (ESC.Watts/360000);

	MPPT1.WattHrs += (MPPT1.Watts/360000);
	MPPT2.WattHrs += (MPPT2.Watts/360000);

	BMU.WattHrs += (BMU.Watts/360000);

	STATS.ODOMETER += ESC.Velocity_KMH/360000;


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
** Function name:		menu_mppt_poll
**
** Description:			1. Sends request packet to MPPT (125K CAN Bus)
** 						2. Sends previous MPPT packet to car (500K CAN Bus)
** 						3. Receives new packet and extracts data (125K CAN Bus)
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menu_mppt_poll (void)
{
	STATS.MPPT_POLL_COUNT ^= 0b1; 								// Toggle bit. Selects which MPPT to poll this round

	// Send request to MPPT
	if((LPC_CAN2->GSR & (1 << 3)) && STATS.MPPT_POLL_COUNT)		// If previous transmission is complete, send message;
	{
		MsgBuf_TX2.MsgID = MPPT2_BASE; 						// Explicit Standard ID
		CAN2_SendMessage( &MsgBuf_TX2 );
	}

	else if(LPC_CAN2->GSR & (1 << 3))							// If previous transmission is complete, send message;
	{
		MsgBuf_TX2.MsgID = MPPT1_BASE; 						// Explicit Standard ID
		CAN2_SendMessage( &MsgBuf_TX2 );
	}

	// Push previous data to car
	if((LPC_CAN1->GSR & (1 << 3)) && STATS.MPPT_POLL_COUNT)		// If previous transmission is complete, send message;
	{
		MsgBuf_TX1.Frame = 0x00070000; 							// 11-bit, no RTR, DLC is 8 bytes
		MsgBuf_TX1.MsgID = MPPT2_BASE + MPPT_RPLY; 						// Explicit Standard ID
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
		MsgBuf_TX1.Frame = 0x00070000; 							// 11-bit, no RTR, DLC is 8 bytes
		MsgBuf_TX1.MsgID = MPPT1_BASE + MPPT_RPLY; 						// Explicit Standard ID
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

	// Receive new MPPT data
	if ( CAN2RxDone == TRUE )
	{
		CAN2RxDone = FALSE;
		if		(MsgBuf_RX2.MsgID == MPPT1_BASE + MPPT_RPLY){mppt_data_extract(&MPPT1, &fakeMPPT1);}
		else if	(MsgBuf_RX2.MsgID == MPPT2_BASE + MPPT_RPLY){mppt_data_extract(&MPPT2, &fakeMPPT2);}

		// Everything is correct, reset buffer
		MsgBuf_RX2.Frame = 0x0;
		MsgBuf_RX2.MsgID = 0x0;
		MsgBuf_RX2.DataA = 0x0;
		MsgBuf_RX2.DataB = 0x0;
	} // Message on CAN 2 received

	// Check mppt connection timeout
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
** Function name:		mppt_data_extract
**
** Description:			Extracts data from CAN 2 Receive buffer into MPPT structure.
**
** Parameters:			1. Address of MPPT to extract to
** 						2. Address of fakeMPPT to use for retransmit
** Returned value:		None
**
******************************************************************************/
void mppt_data_extract (MPPT *_MPPT, fakeMPPTFRAME *_fkMPPT)
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

	// Update the global variables after IIR filtering
	_MPPT->Tmp = iirFILTER(((_Data_B & 0xFF0000) >> 16), _MPPT->Tmp, 8); //infinite impulse response filtering
	_MPPT->VIn = iirFILTER(_VIn, _MPPT->VIn, IIR_FILTER_GAIN);
	_MPPT->IIn = iirFILTER(_IIn, _MPPT->IIn, IIR_FILTER_GAIN);
	_MPPT->VOut = iirFILTER(_VOut, _MPPT->VOut, IIR_FILTER_GAIN);
	if(_MPPT->Connected<2){_MPPT->Connected = 2;}
}

/******************************************************************************
** Function name:		menu_input_check
**
** Description:			Checks majority of inputs (Switches, Left, Right)
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menu_input_check (void)
{
	unsigned char OLD_IO = SWITCH_IO;

	SWITCH_IO = 0;
	SWITCH_IO |= (FORWARD << 0);
	SWITCH_IO |= (REVERSE << 1);
	SWITCH_IO |= (SPORTS_MODE << 2);
	SWITCH_IO |= (LEFT_ON << 3);
	SWITCH_IO |= (RIGHT_ON << 4);

	if(OLD_IO != SWITCH_IO){buzzer(50);}	// BEEP if toggle position has changed.

	if(RIGHT)
	{
		MENU.MENU_POS++;delayMs(1, 400);buzzer(50);
		if(MENU.MENU_POS>=MENU_ITEMS){MENU.MENU_POS = 0;}
		if(MENU.MENU_POS==1){buzzer(300);}
		if((ESC.ERROR & 0x2) && !STATS.SWOC_ACK){STATS.SWOC_ACK = TRUE;}
		if((ESC.ERROR & 0x1) && !STATS.HWOC_ACK){STATS.HWOC_ACK = TRUE;BUZZER_OFF}
		if(STATS.COMMS == 1)
		{
			if((LPC_CAN1->GSR & (1 << 3)))				// If previous transmission is complete, send message;
			{
				MsgBuf_TX1.Frame = 0x00010000; 			/* 11-bit, no RTR, DLC is 1 byte */
				MsgBuf_TX1.MsgID = DASH_RPLY + 1; 		/* Explicit Standard ID */
				MsgBuf_TX1.DataA = 0x0;
				MsgBuf_TX1.DataB = 0x0;
				CAN1_SendMessage( &MsgBuf_TX1 );
			}
			STATS.COMMS = 0;
		}

		lcd_clear();
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
		if(STATS.COMMS == 1)
		{
			if((LPC_CAN1->GSR & (1 << 3)))				// If previous transmission is complete, send message;
			{
				MsgBuf_TX1.Frame = 0x00010000; 			/* 11-bit, no RTR, DLC is 1 byte */
				MsgBuf_TX1.MsgID = DASH_RPLY + 1; 		/* Explicit Standard ID */
				MsgBuf_TX1.DataA = 0x0;
				MsgBuf_TX1.DataB = 0x0;
				CAN1_SendMessage( &MsgBuf_TX1 );
			}
			STATS.COMMS = 0;
		}

		lcd_clear();
		MENU.SELECTED = 0;
		MENU.ITEM_SELECTOR = 0;
		MENU.SUBMENU_POS = 0;
	}

	// Check Sports/Economy switch
	if(SWITCH_IO & 0x4)	{STATS.DRIVE_MODE = SPORTS;STATS.RAMP_SPEED = SPORTS_RAMP_SPEED;}
	else				{STATS.DRIVE_MODE = ECONOMY;STATS.RAMP_SPEED = ECONOMY_RAMP_SPEED;}
}

/******************************************************************************
** Function name:		menu_fault_check
**
** Description:			Checks for faults in car components
**
** Parameters:			None
** Returned value:		Fault status
** 							0 - No fault
** 							1 - Non critical fault
** 							2 - Critical fault - cancel drive
**
******************************************************************************/
int menu_fault_check (void)
{ // TODO: POWERGOOD pin? Critical fault?
	if (ESC.ERROR || (BMU.Status & 0xD37)){return 2;}
	if (MPPT1.UNDV || MPPT1.OVT || MPPT2.UNDV || MPPT2.OVT || (BMU.Status & 0x1288)){return 1;}
	return 0;
}

/******************************************************************************
** Function name:		menu_drive
**
** Description:			Reads drive inputs and configures drive packet
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menu_drive (void)
{
	uint32_t ADC_A;
	uint32_t ADC_B;

	/// THROTTLE
	if(!STATS.CR_ACT)
	{
		ADC_A = (ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0) + ADCRead(0))/8;

		THR_POS = (1500 - ADC_A);
		THR_POS = (THR_POS * 9)/10;
		if(THR_POS < 0){THR_POS = 0;}
		if(THR_POS > 1000){THR_POS = 1000;}
	}

	/// REGEN
	ADC_B = (ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1) + ADCRead(1))/8;

	// Calibrate endpoints and add deadband.
	RGN_POS = (ADC_B - 1660);
	RGN_POS = (RGN_POS * 9)/10;
	if(RGN_POS < 0){RGN_POS = 0;}
	if(RGN_POS > 1000){RGN_POS = 1000;}
	if(RGN_POS){STATS.CR_ACT = OFF;}

	// MinorSec: DRIVE LOGIC
	if (!MECH_BRAKE && (FORWARD || REVERSE)){
		if(STATS.CR_ACT){DRIVE.Current = 1.0; DRIVE.Speed_RPM = STATS.CRUISE_SPEED / ((60 * 3.14 * WHEEL_D_M) / 1000.0);}
		else if(!THR_POS && !RGN_POS){DRIVE.Speed_RPM = 0; DRIVE.Current = 0;}
		else if(FORWARD && ESC.Velocity_KMH > -5.0 && !RGN_POS && ((DRIVE.Current * 1000) < THR_POS))	{DRIVE.Speed_RPM = 1500; 	DRIVE.Current += (STATS.RAMP_SPEED / 1000.0);}
		else if(FORWARD && ESC.Velocity_KMH > -5.0 && !RGN_POS)											{DRIVE.Speed_RPM = 1500; 	DRIVE.Current = (THR_POS / 1000.0);}
		else if(REVERSE && ESC.Velocity_KMH < 1.0 && !RGN_POS && ((DRIVE.Current * 1000) < THR_POS))	{DRIVE.Speed_RPM = -200; 	DRIVE.Current += (STATS.RAMP_SPEED / 1000.0);}
		else if(REVERSE && ESC.Velocity_KMH < 1.0 && !RGN_POS)											{DRIVE.Speed_RPM = -200; 	DRIVE.Current = (THR_POS / 1000.0);}
		else if(RGN_POS && ((DRIVE.Current * 1000) < RGN_POS))											{DRIVE.Speed_RPM = 0; 		DRIVE.Current += (REGEN_RAMP_SPEED / 1000.0);}
		else if(RGN_POS)																				{DRIVE.Speed_RPM = 0; 		DRIVE.Current = (RGN_POS / 2);}
		else{DRIVE.Speed_RPM = 0; DRIVE.Current = 0;}}
	else{DRIVE.Speed_RPM = 0; DRIVE.Current = 0;}
}

/******************************************************************************
** Function name:		menu_lights
**
** Description:			Controls status of lights and LEDs
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menu_lights (void)
{
	if(MECH_BRAKE || RGN_POS){BRAKELIGHT_ON;}
	else{BRAKELIGHT_OFF;}

	if(!RGN_POS)
	{
		if(REVERSE){REVERSE_ON;NEUTRAL_OFF;REGEN_OFF;DRIVE_OFF;STATS.IGNITION = 0x21;}
		else if(FORWARD){REVERSE_OFF;NEUTRAL_OFF;REGEN_OFF;DRIVE_ON;STATS.IGNITION = 0x28;}
		else{REVERSE_OFF;NEUTRAL_ON;REGEN_OFF;DRIVE_OFF;STATS.IGNITION = 0x22;}
	}
	else
	{
		if(REVERSE){REVERSE_ON;NEUTRAL_OFF;REGEN_ON;DRIVE_OFF;STATS.IGNITION = 0x25;}
		else if(FORWARD){REVERSE_OFF;NEUTRAL_OFF;REGEN_ON;DRIVE_ON;STATS.IGNITION = 0x2C;}
		else{REVERSE_OFF;NEUTRAL_ON;REGEN_OFF;DRIVE_OFF;STATS.IGNITION = 0x22;}
	}

	if ((SWITCH_IO & 0x8) && (CLOCK.T_mS > 25 && CLOCK.T_mS < 75)){BLINKER_L_ON}
	else {BLINKER_L_OFF}
	if ((SWITCH_IO & 0x10) && (CLOCK.T_mS > 25 && CLOCK.T_mS < 75)){BLINKER_R_ON}
	else {BLINKER_R_OFF}

	if(SWITCH_IO & 0x4)	{SPORTS_ON;ECO_OFF}
	else				{SPORTS_OFF;ECO_ON}

	if(STATS.FAULT == 1){FAULT_ON}
	else if(STATS.FAULT == 2 && (CLOCK.T_mS > 25 && CLOCK.T_mS < 75)){FAULT_ON}
	else{FAULT_OFF}
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
	/*
	 * Handle custom packets in can.c, flag incoming flag type to check here.
	 * Continue using this to send response packets
	 *
	 * Kill drive: use same loop to prevent drive logic running
	 * Comm check: flag for comms check to be picked up in menu loop
	 *
	 * Old power packets should be redundant if telemetry reads every CAN packet
	 */
	if ( CAN1RxDone == TRUE )
	{
		CAN1RxDone = FALSE;

		if((MsgBuf_RX1.MsgID == DASH_RQST) && (MsgBuf_RX1.DataA == 0x4C4C494B) && (MsgBuf_RX1.DataB == 0x45565244)) // Data = KILLDRVE
		{
			lcd_clear();
			char buffer[20];

			sprintf(buffer, "GONEROGUE?");
			_lcd_putTitle(buffer);

			sprintf(buffer, "--   KILL DRIVE   --");
			lcd_putstring(1,0, buffer);
			lcd_putstring(2,0, buffer);
			lcd_putstring(3,0, buffer);

			while(FORWARD || REVERSE)
			{
				buzzer(1000);
				delayMs(1,500);
			}
			lcd_clear();
		}

		// Clear Rx Buffer
		MsgBuf_RX1.Frame = 0x0;
		MsgBuf_RX1.MsgID = 0x0;
		MsgBuf_RX1.DataA = 0x0;
		MsgBuf_RX1.DataB = 0x0;
	}
}

/******************************************************************************
** Function name:		menu_calc
**
** Description:			Calculates instantaneous values and peaks
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menu_calc (void)
{
	// Calculate Power of components
	ESC.Watts = ESC.Bus_V * ESC.Bus_I;

	BMU.Watts = BMU.Battery_I * BMU.Battery_V;

	MPPT1.Watts = (MPPT1.VIn * MPPT1.IIn) / 1000.0;
	MPPT2.Watts = (MPPT2.VIn * MPPT2.IIn) / 1000.0;

	// Check peaks
	if(ESC.Watts > ESC.MAX_Watts){ESC.MAX_Watts = ESC.Watts;}
	if(ESC.Bus_I > ESC.MAX_Bus_I){ESC.MAX_Bus_I = ESC.Bus_I;}
	if(ESC.Bus_V > ESC.MAX_Bus_V){ESC.MAX_Bus_V = ESC.Bus_V;}
	if(ESC.Velocity_KMH > STATS.MAX_SPEED){STATS.MAX_SPEED = ESC.Velocity_KMH;}

	if(MPPT1.Tmp > MPPT1.MAX_Tmp){MPPT1.MAX_Tmp = MPPT1.Tmp;}
	if(MPPT1.VIn > MPPT1.MAX_VIn){MPPT1.MAX_VIn = MPPT1.VIn;}
	if(MPPT1.IIn > MPPT1.MAX_IIn){MPPT1.MAX_IIn = MPPT1.IIn;}
	if(MPPT1.Watts > MPPT1.MAX_Watts){MPPT1.MAX_Watts = MPPT1.Watts;}

	if(MPPT2.Tmp > MPPT2.MAX_Tmp){MPPT2.MAX_Tmp = MPPT2.Tmp;}
	if(MPPT2.VIn > MPPT2.MAX_VIn){MPPT2.MAX_VIn = MPPT2.VIn;}
	if(MPPT2.IIn > MPPT2.MAX_IIn){MPPT2.MAX_IIn = MPPT2.IIn;}
	if(MPPT2.Watts > MPPT2.MAX_Watts){MPPT2.MAX_Watts = MPPT2.Watts;}

	if(BMU.Watts > BMU.MAX_Watts){BMU.MAX_Watts = BMU.Watts;}
	if(BMU.Battery_I > BMU.MAX_Battery_I){BMU.MAX_Battery_I = BMU.Battery_I;}
	if(BMU.Battery_V > BMU.MAX_Battery_V){BMU.MAX_Battery_V = BMU.Battery_V;}
}

/******************************************************************************
** Function name:		recallVariables
**
** Description:			Restores persistent variables from EEPROM
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void recallVariables (void)
{
	// Restore Non-Volatile Data
	STATS.BUZZER = EE_Read(AddressBUZZ);

	if(EE_Read(AddressBL) < 2000){PWMBL = EE_Read(AddressBL);}
	else{PWMBL = 500;EE_Write(AddressBL, PWMBL);}

	STATS.ODOMETER = conv_uint_float(EE_Read(AddressODO));
	STATS.MAX_SPEED = 0;
	STATS.RAMP_SPEED = 5;
	STATS.CRUISE_SPEED = 0;
	STATS.CR_ACT = 0;
	STATS.CR_STS = 0;

	BMU.WattHrs = conv_uint_float(EE_Read(AddressBMUWHR));
	MPPT1.WattHrs = conv_uint_float(EE_Read(AddressMPPT1WHR));
	MPPT2.WattHrs = conv_uint_float(EE_Read(AddressMPPT2WHR));
}

/******************************************************************************
** Function name:		storeVariables
**
** Description:			Saves persistent variables to EEPROM
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void storeVariables (void)
{
	if(CLOCK.T_S % 2)
	{
		EE_Write(AddressODO, conv_float_uint(STATS.ODOMETER));
		delayMs(1,3);
	}

	else
	{
		EE_Write(AddressBMUWHR, conv_float_uint(BMU.WattHrs));
		delayMs(1,3);
		EE_Write(AddressMPPT1WHR, conv_float_uint(MPPT1.WattHrs));
		delayMs(1,3);
		EE_Write(AddressMPPT2WHR, conv_float_uint(MPPT2.WattHrs));
		delayMs(1,3);
	}
}

/******************************************************************************
** Function name:		EE_Read
**
** Description:			Reads a word from EEPROM (Uses I2CRead)
**
** Parameters:			Address to read from
** Returned value:		Data at address
**
******************************************************************************/
uint32_t EE_Read (uint32_t _EEadd)
{
	uint32_t retDATA = 0;

	retDATA = I2C_Read(_EEadd+3);
	retDATA = (retDATA << 8) + I2C_Read(_EEadd+2);
	retDATA = (retDATA << 8) + I2C_Read(_EEadd+1);
	retDATA = (retDATA << 8) + I2C_Read(_EEadd+0);

	return retDATA;
}

/******************************************************************************
** Function name:		EE_Write
**
** Description:			Saves a word to EEPROM (Uses I2CWrite)
**
** Parameters:			1. Address to save to
** 						2. Data to save
** Returned value:		None
**
******************************************************************************/
void EE_Write (uint32_t _EEadd, uint32_t _EEdata)
{
	uint32_t temp1 = (_EEdata & 0x000000FF);
	uint32_t temp2 = (_EEdata & 0x0000FF00) >> 8;
	uint32_t temp3 = (_EEdata & 0x00FF0000) >> 16;
	uint32_t temp4 = (_EEdata & 0xFF000000) >> 24;

	I2C_Write(_EEadd, temp1,temp2,temp3,temp4);
}

/******************************************************************************
** Function name:		I2C_Read
**
** Description:			Reads a word from EEPROM
**
** Parameters:			Address to read from
** Returned value:		Data at address
**
******************************************************************************/
uint32_t I2C_Read (uint32_t _EEadd)
{
	int i;

	for ( i = 0; i < BUFSIZE; i++ )	  	/* clear buffer */
	{
		I2CMasterBuffer[PORT_USED][i] = 0;
	}

	I2CWriteLength[PORT_USED] = 3;
	I2CReadLength[PORT_USED] = 1;
	I2CMasterBuffer[PORT_USED][0] = _24LC256_ADDR;
	I2CMasterBuffer[PORT_USED][1] = 0x00;	/* address */
	I2CMasterBuffer[PORT_USED][2] = _EEadd;		/* address */
	I2CMasterBuffer[PORT_USED][3] = _24LC256_ADDR | RD_BIT;
	I2CEngine( PORT_USED );
	I2CStop(PORT_USED);

	return (uint32_t)I2CSlaveBuffer[PORT_USED][0];
}

/******************************************************************************
** Function name:		I2C_Write
**
** Description:			Saves a word to EEPROM
**
** Parameters:			1. Address to save to
** 						2. Data to save
** Returned value:		None
**
******************************************************************************/
void I2C_Write (uint32_t _EEadd, uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3)
{
	I2CWriteLength[PORT_USED] = 7;
	I2CReadLength[PORT_USED] = 0;
	I2CMasterBuffer[PORT_USED][0] = _24LC256_ADDR;
	I2CMasterBuffer[PORT_USED][1] = 0x00;			/* address */
	I2CMasterBuffer[PORT_USED][2] = _EEadd;	/* address */
	I2CMasterBuffer[PORT_USED][3] = data0;
	I2CMasterBuffer[PORT_USED][4] = data1;
	I2CMasterBuffer[PORT_USED][5] = data2;
	I2CMasterBuffer[PORT_USED][6] = data3;
	I2CEngine( PORT_USED );

	delayMs(1,10);
}

/******************************************************************************
** Function name:		iirFILTER
**
** Description:			Filter to flatten out erratic data reads
**
** Parameters:			1. Input data
** 						2. Existing data
** 						3. Gain factor - Default value -> IIR_FILTER_GAIN
** Returned value:		Smoothed value
**
******************************************************************************/
uint32_t iirFILTER (uint32_t _data_in, uint32_t _cur_data, uint8_t _gain)
{return (((_gain-1)*_cur_data)+_data_in)/_gain;}

/******************************************************************************
** Function name:		init_GPIO
**
** Description:			Configures pins to be used for GPIO
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void init_GPIO (void)
{
	/* GPIO0:
	 * 	PINSEL0:
	 * 		0 - IN - RIGHT
	 * 		1 - IN - INC/UP
	 * 		3 - OUT - Buzzer
	 * 		10 - IN - REVERSE
	 * 		11 - IN - FORWARD
	 * 		15 - OUT - LCD Reset
	 * 	PINSEL1:
	 * 		16 - OUT - LCD Enable
	 * 		25 - IN - Mech Brake
	 * 		27 - OUT - Fault LED
	 */
	LPC_GPIO0->FIODIR = (1<<3)|(1<<15)|(1<<16)|(1<<27);
	LPC_GPIO0->FIOCLR = (1<<3)|(1<<15)|(1<<16)|(1<<27);

	/*
	 * GPIO1:
	 * 	PINSEL2:
	 * 		0 - IN - RIGHT_ON
	 * 		1 - IN - LEFT_ON
	 * 		4 - IN - Power Status
	 * 		8 - IN - Armed Status
	 * 	PINSEL3:
	 * 		19 - OUT - Blinker R
	 * 		20 - OUT - Blinker L
	 * 		21 - OUT - Brake Light
	 * 		23 - OUT - Reverse LED
	 * 		24 - OUT - Neutral LED
	 * 		25 - OUT - Regen LED
	 * 		26 - OUT - Drive LED
	 * 		27 - IN - LEFT
	 * 		28 - IN - DEC/DOWN
	 * 		29 - IN - SELECT
	 * 		30 - OUT - ECO LED
	 * 		31 - OUT - SPORTS LED
	 */
	LPC_GPIO1->FIODIR = (1<<19)|(1<<20)|(1<<21)|(1<<23)|(1<<24)|(1<<25)|(1<<26)|(1<<30)|(1<<31);
	LPC_GPIO1->FIOCLR = (1<<19)|(1<<20)|(1<<21)|(1<<23)|(1<<24)|(1<<25)|(1<<26)|(1<<30)|(1<<31);

	/*
	 * GPIO2:
	 * 	PINSEL4:
	 * 		6 - OUT - LCD D7
	 * 		7 - OUT - LCD D6
	 * 		8 - OUT - LCD D5
	 * 		9 - OUT - LCD D4
	 * 		10 - IN - Aux ON (SPORTS MODE)
	 * 		11 - IN - Aux OFF
	 * 		12 - IN - Spare switch
	 * 		13 - IN - Spare switch
	 */
	LPC_GPIO2->FIODIR = (1<<6)|(1<<7)|(1<<8)|(1<<9);
	LPC_GPIO2->FIOCLR = (1<<6)|(1<<7)|(1<<8)|(1<<9);

	/*
	 * GPIO3:
	 * 	PINSEL7:
	 * 		25 - OUT - Left LED
	 * 		26 - OUT - Right LED
	 */
	LPC_GPIO3->FIODIR = (1<<25)|(1<<26);
	LPC_GPIO3->FIOCLR = (1<<25)|(1<<26);
}

/******************************************************************************
** Function name:		buzzer
**
** Description:			Turns buzzer on for set amount of time
**
** Parameters:			Time to activate buzzer
** Returned value:		None
**
******************************************************************************/
void buzzer (uint32_t val)
{
	if(STATS.BUZZER)
	{BUZZER_ON;}
	delayMs(1,val);
	BUZZER_OFF;
}

/******************************************************************************
** Function name:		BOD_Init
**
** Description:			Configures BOD Handler
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void BOD_Init ( void )
{
	/* Turn on BOD. */
	LPC_SC->PCON &= ~(0x1<<3);

	/* Enable the BOD Interrupt */
	NVIC_EnableIRQ(BOD_IRQn);
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

	init_GPIO();

	setLCD();
	lcd_clear();

	I2C1Init();

	PWM_Init(1, 1300);		//1300 PWM steps. 1000/1300 gives roughly 5volts on the A-OUT Ports.
	PWM_Start(1);

	setCANBUS1();
	setCANBUS2();

	BOD_Init();

	lcd_display_intro();

	recallVariables();

	menuInit();

	while (FORWARD || REVERSE)
	{
		lcd_display_errOnStart();
		DUTYBL = 1000; // TODO: No backlight on OLED
		buzzer(700);

		lcd_clear();
		DUTYBL = 0;
		delayMs(1, 300);
	}

	while(1) { // Exiting this loop ends the program
		if((ESC.ERROR & 0x2) && !STATS.SWOC_ACK) // on unacknowledged SWOC error, show error screen
		{MENU.errors[0]();}
		else if ((ESC.ERROR & 0x1) && !STATS.HWOC_ACK) // on unacknowledged SWOC error, show error screen
		{MENU.errors[1]();}
		else if (STATS.COMMS)
		{MENU.errors[2]();}
		else
		{
			if(STATS.SWOC_ACK && !(ESC.ERROR & 0x2)) // if acknowledged previous error is reset
			{STATS.SWOC_ACK = FALSE;}
			if(STATS.HWOC_ACK && !(ESC.ERROR & 0x1)) // if acknowledged previous error is reset
			{STATS.HWOC_ACK = FALSE;}

			MENU.menus[MENU.MENU_POS]();
		}

		menu_mppt_poll();
		menu_input_check();
		if((STATS.FAULT = menu_fault_check()) != 2){menu_drive();} // no drive on fault code 2
		menu_lights();
		tx500CAN();
		menu_calc();
    }

    return 0; // For compilers sanity
}

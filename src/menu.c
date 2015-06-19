/*
 * menu.c
 *
 *  Created on: 29 May 2015
 *      Author: Stuff
 *
 *
 *  lcd_display_<menu>
 */

#include <stdio.h>
#include <stdint.h>
#include "menu.h"
#include "dash.h"

#include "lpc17xx.h"

extern MOTORCONTROLLER ESC1, ESC2;
extern MPPT MPPT1, MPPT2;
extern CAN_MSG MsgBuf_TX1;
extern uint16_t PWMBL;
extern uint16_t THR_POS, RGN_POS;


//////////////////////////////////////////////
/// Not in array, reference manually

/******************************************************************************
** Function name:		lcd_display_errOnStart
**
** Description:			Error screen on boot
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_errOnStart (void)
{
	lcd_putstring(0,0, "--    CAUTION!    --");
	lcd_putstring(1,0, "                    ");
	lcd_putstring(2,0, "   GEARS ENGAGED!   ");
	lcd_putstring(3,0, "                    ");
}

/******************************************************************************
** Function name:		lcd_display_intro
**
** Description:			Boot intro screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_intro (void)
{
	lcd_putstring(0,0, "**  UWS WSC 2015  **");
	lcd_putstring(1,0, "                    ");
	lcd_putstring(2,0, "   NEWCAR Driver    ");
	lcd_putstring(3,0, "   Interface v2.0   ");
	delayMs(1,1600);

	lcd_putstring(0,0, "**  UWS WSC 2015  **");
	lcd_putstring(1,0, "                    ");
	lcd_putstring(2,0, "    BUZZER Test..   ");
	lcd_putstring(3,0, "                    ");
	BUZZER_ON
	delayMs(1,100);
	BUZZER_OFF

	lcd_clear();
}
///////////////////////////////////////////////

///////////////////////////////////////////////
/// menus array

/******************************************************************************
** Function name:		lcd_display_info
**
** Description:			Car information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_info (void) // menus[0]
{
	char buffer[20];

	sprintf(buffer, "-INFO-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "SOLACE DashBOARD2.0 ");
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "HW Version: 2.0a    ");
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "SW Version: 2.0a    ");
	lcd_putstring(3,0, buffer);
}

/******************************************************************************
** Function name:		lcd_display_escBus
**
** Description:			Data screen for precharge
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_escBus (void) // menus[1]
{// TODO: CHANGE TO CONTROL PRECHARGE
	char buffer[20];

	sprintf(buffer, "-ESC BUS-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "BUS VOLTAGE: %03.0f V  ", ESC.Bus_V);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "BAT VOLTAGE: %03lu V  ", BMU.Battery_I);
	lcd_putstring(2,0, buffer);

	if((BMU.Battery_V - ESC.Bus_V) < 10)
	{
		sprintf(buffer, "  **SAFE TO BOOT**  ");
		lcd_putstring(3,0, buffer);
	}
	else
	{
		sprintf(buffer, "                    ");
		lcd_putstring(3,0, buffer);
	}
}

/******************************************************************************
** Function name:		lcd_display_home
**
** Description:			Speed, drive, array power, basic errors
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_home (void) // menus[2]
{
	char buffer[20];

	sprintf(buffer, "-HOME-");
	_lcd_putTitle(buffer);

	if(STATS.DRIVE_MODE == SPORTS)
	{
		sprintf(buffer, "DRIVE MODE:  SPORTS ");
		lcd_putstring(1,0, buffer);
	}

	else
	{
		sprintf(buffer, "DRIVE MODE: ECONOMY ");
		lcd_putstring(1,0, buffer);
	}

	// TODO: Unlikely to get throttle % from controller, can get absolute current.
	// May be able to hard code current cap for controller and divide for %
	if(STATS.CR_ACT)
	{
		if (CRUISE.CO >= 1000){sprintf(buffer, "CRUISE FW: %3d.%d%%   ", PWMA/10,PWMA%10);}
		else{sprintf(buffer, "CRUISE RG: %3d.%d%%   ", PWMC/10,PWMC%10);}

		lcd_putstring(2,0, buffer);
	}
	else if(FORWARD && !RGN_POS){
		sprintf(buffer, "DRIVE:     %3d.%d%%   ", THR_POS/10,THR_POS%10);
		lcd_putstring(2,0, buffer);}

	else if(REVERSE && !RGN_POS){
		sprintf(buffer, "REVERSE:   %3d.%d%%   ", THR_POS/10,THR_POS%10);
		lcd_putstring(2,0, buffer);}

	else if(RGN_POS){
		sprintf(buffer, "REGEN:     %3d.%d%%   ", RGN_POS/10,RGN_POS%10);
		lcd_putstring(2,0, buffer);	}

	else{
		sprintf(buffer, "NEUTRAL:   %3d.%d%%   ", THR_POS/10,THR_POS%10);
		lcd_putstring(2,0, buffer);}

	// If no ERRORS then display MPPT Watts
	// TODO: ??? check all errors?

	if(ESC.ERROR)
	{
		sprintf(buffer, "ESC  FAULT          ");
		lcd_putstring(3,0, buffer);
	}

	else
	{
		sprintf(buffer, "ARRAY:    %3.1f W  ", MPPT2.Watts+MPPT1.Watts);
		lcd_putstring(3,0, buffer);
	}
}

/******************************************************************************
** Function name:		lcd_display_drive
**
** Description:			Drive details screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_drive (void) // menus[3]
{
	char buffer[20];

	sprintf(buffer, "-CONTROLS-");
	_lcd_putTitle(buffer);

	if(FORWARD){
	sprintf(buffer, "MODE:          DRIVE");
	lcd_putstring(1,0, buffer);}

	else if(REVERSE){
	sprintf(buffer, "MODE:        REVERSE");
	lcd_putstring(1,0, buffer);}

	else{
	sprintf(buffer, "MODE:        NEUTRAL");
	lcd_putstring(1,0, buffer);}

	if(!RGN_POS)
	{
		sprintf(buffer, "OUTPUT:       %3.1f%%", DRIVE.Current*100);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "THROTTLE:     %3d.%d%%", THR_POS/10,THR_POS%10);
		lcd_putstring(3,0, buffer);
	}

	else
	{
		sprintf(buffer, "OUTPUT:       %3.1f%%", DRIVE.Current*100);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "REGEN:        %3d.%d%%", RGN_POS/10,RGN_POS%10);
		lcd_putstring(3,0, buffer);
	}
}

/******************************************************************************
** Function name:		lcd_display_cruise
**
** Description:			Cruise control screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_cruise (void) // menus[4]
{// TODO: MAJOR REWORK FOR WS CRUISE
	char buffer[20];

	if (MENU.SUBMENU_POS == 0) // Cruise Menu
	{
		sprintf(buffer, "-CRUISE-");
		_lcd_putTitle(buffer);
		if (!REVERSE)
		{
			if (STATS.CR_STS && STATS.CR_ACT)
			{
				sprintf(buffer, " STS:  ON  ACT:  ON ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET: %3.0f  THR:%3d%% ", STATS.CRUISE_SPEED, PWMA/10);
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:%3d%% ", ESC.Velocity_KMH, PWMC/10);
				lcd_putstring(3,0, buffer);

				// Button presses
				if (SELECT){STATS.CR_ACT = OFF;}
				else if (INCREMENT){STATS.CRUISE_SPEED += 1;}
				else if (DECREMENT){STATS.CRUISE_SPEED -= 1;}
			}
			else if (STATS.CR_STS && !STATS.CR_ACT)
			{
				sprintf(buffer, " STS:  ON  ACT: OFF ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET: %3.0f  THR:     ", STATS.CRUISE_SPEED);
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:     ", ESC.Velocity_KMH);
				lcd_putstring(3,0, buffer);

				// Button presses
				if (SELECT){STATS.CR_STS = OFF;STATS.CRUISE_SPEED = 0;}
				else if (INCREMENT && (STATS.CRUISE_SPEED > 1)){STATS.CR_ACT = ON;}
				else if (DECREMENT){STATS.CRUISE_SPEED = ESC.Velocity_KMH;STATS.CR_ACT = ON;}
			}
			else if (STATS.CR_ACT && !STATS.CR_STS) // Should never trip, but just in case
			{
				sprintf(buffer, " STS: OFF  ACT:  ON ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, "    CRUISE ERROR    ");
				lcd_putstring(2,0, buffer);

				sprintf(buffer, "     RESETTING      ");
				lcd_putstring(3,0, buffer);

				STATS.CR_ACT = OFF;
				STATS.CR_STS = OFF;
				STATS.CRUISE_SPEED = 0;
			}
			else
			{
				sprintf(buffer, " STS: OFF  ACT: OFF ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET:      THR:     ");
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:     ", ESC.Velocity_KMH);
				lcd_putstring(3,0, buffer);

				// Button presses
				if (SELECT){STATS.CRUISE_SPEED = 0;STATS.CR_STS = ON;}
				else if (INCREMENT || DECREMENT){MENU.SUBMENU_POS++;}
			}
		}
		else // no cruise in reverse
		{
			sprintf(buffer, " STS: OFF  ACT: OFF ");
			lcd_putstring(1,0, buffer);

			sprintf(buffer, "  REVERSE ENGAGED!  ");
			lcd_putstring(2,0, buffer);

			sprintf(buffer, "                    ");
			lcd_putstring(3,0, buffer);

			STATS.CR_STS = OFF;
			STATS.CR_ACT = OFF;
			STATS.CRUISE_SPEED = 0;
		}
	}
	else if (MENU.SUBMENU_POS == 1)
	{
		sprintf(buffer, "-CRU TEST-");
		_lcd_putTitle(buffer);

		if (MENU.SELECTED || (CLOCK.T_S % 2))
		{
			switch (MENU.ITEM_SELECTOR)
			{
				default:
					MENU.ITEM_SELECTOR = 0;
					/* no break */
				case 0:
					sprintf(buffer, "THR:%04.0f << SPD:%5.1f ", CRUISE.SP, ESC.Velocity_KMH);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET                 ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET           EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 1:
					sprintf(buffer, "THR:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC.Velocity_KMH);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET      <<         ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET           EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 2:
					sprintf(buffer, "THR:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC.Velocity_KMH);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET                 ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET    <<     EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 3:
					sprintf(buffer, "THR:%04.0f    SPD:%5.1f ", CRUISE.SP, ESC.Velocity_KMH);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET                 ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET        >> EXIT");
					lcd_putstring(3,0, buffer);
					break;
			}
		}
		else
		{
			sprintf(buffer, "THR:%04.0f    SPD:%5.1f ", CRUISE.SP, ESC.Velocity_KMH);
			lcd_putstring(1,0, buffer);

			sprintf(buffer, "SET                 ");
			lcd_putstring(2,0, buffer);

			sprintf(buffer, "RESET           EXIT");
			lcd_putstring(3,0, buffer);
		}

		if (MENU.SELECTED) // Button checks
		{
			if (SELECT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 0:
						// deselect menu
						break;
					case 1:
						// set CO
						CRUISE.CO = CRUISE.SP;
						break;
					case 2:
						// reset SP
						CRUISE.SP = CRUISE.CO;
						break;
					default:
					case 3: // exit
						CRUISE.SP = 0;
						CRUISE.CO = 0;
						MENU.SUBMENU_POS = 0;
						MENU.ITEM_SELECTOR = 0;
						break;
				}
				MENU.SELECTED = 0;
			}
			else if (INCREMENT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 0:
						// inc SP
						CRUISE.SP += 1;
						break;
					default:
					case 1:
					case 2:
					case 3:
						MENU.SELECTED = 0;
						break;
				}
			}
			else if (DECREMENT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 0:
						// dec CO
						CRUISE.SP -= 1;
						break;
					default:
					case 1:
					case 2:
					case 3:
						MENU.SELECTED = 0;
						break;
				}
			}
		}
		else
		{
			if (SELECT)
			{
				MENU.ITEM_SELECTOR = 3;
			}
			else if (INCREMENT)
			{
				MENU.ITEM_SELECTOR++;
				if(MENU.ITEM_SELECTOR > 3){MENU.ITEM_SELECTOR = 0;}
			}
			else if (DECREMENT)
			{
				MENU.ITEM_SELECTOR--;
				if(MENU.ITEM_SELECTOR < 0){MENU.ITEM_SELECTOR = 3;}
			}
		}

	}
}

/******************************************************************************
** Function name:		lcd_display_MPPT1
**
** Description:			MPPT1 information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_MPPT1(void) // menus[5]
{
	char buffer[20];

	sprintf(buffer, "-MPPT 1-");
	_lcd_putTitle(buffer);

	if(MPPT1.Connected)
	{
		sprintf(buffer, "IN: %3lu.%luV @ %2lu.%02luA ", MPPT1.VIn/10, MPPT1.VIn%10, MPPT1.IIn/100, MPPT1.IIn%100);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "OUT:%3lu.%luV @ %3.1fW ", MPPT1.VOut/10, MPPT1.VOut%10, MPPT1.Watts);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "%2lu%cC", MPPT1.Tmp, 0xDF);
		lcd_putstring(3,16, buffer);

		if(CLOCK.T_mS > 0 && CLOCK.T_mS < 25)
		{
			if(MPPT1.BVLR)
			{
				sprintf(buffer, "BATTERY FULL    ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 50 && CLOCK.T_mS < 75)
		{
			if(MPPT1.UNDV)
			{
				sprintf(buffer, "LOW IN VOLTAGE  ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 25 && CLOCK.T_mS < 50)
		{
			if(MPPT1.OVT)
			{
				sprintf(buffer, "OVER TEMP       ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 75 && CLOCK.T_mS < 100)
		{
			if(MPPT1.NOC)
			{
				sprintf(buffer, "NO BATTERY      ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}
	}

	else
	{
		sprintf(buffer, "                    ");
		lcd_putstring(1,0, buffer);
		sprintf(buffer, "                    ");
		lcd_putstring(3,0, buffer);

		// flash every 1 second if not connected
		if(((CLOCK.T_mS > 0 && CLOCK.T_mS < 25) || (CLOCK.T_mS > 50 && CLOCK.T_mS < 75)))
		{
			sprintf(buffer, "**CONNECTION ERROR**");
			lcd_putstring(2,0, buffer);
		}

		else
		{
			sprintf(buffer, "                    ");
			lcd_putstring(2,0, buffer);
		}
	}
}


/******************************************************************************
** Function name:		lcd_display_MPPT2
**
** Description:			MPPT2 information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_MPPT2(void) // menus[6]
{
	char buffer[20];

	sprintf(buffer, "-MPPT 2-");
	_lcd_putTitle(buffer);

	if(MPPT2.Connected)
	{
		sprintf(buffer, "IN: %3lu.%luV @ %2lu.%02luA ", MPPT2.VIn/10, MPPT2.VIn%10, MPPT2.IIn/100, MPPT2.IIn%100);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "OUT:%3lu.%luV @ %3.1fW ", MPPT2.VOut/10, MPPT2.VOut%10, MPPT2.Watts);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "%2lu%cC", MPPT2.Tmp, 0xDF);
		lcd_putstring(3,16, buffer);

		if(CLOCK.T_mS > 0 && CLOCK.T_mS < 25)
		{
			if(MPPT2.BVLR)
			{
				sprintf(buffer, "BATTERY FULL    ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 50 && CLOCK.T_mS < 75)
		{
			if(MPPT2.UNDV)
			{
				sprintf(buffer, "LOW IN VOLTAGE  ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 25 && CLOCK.T_mS < 50)
		{
			if(MPPT2.OVT)
			{
				sprintf(buffer, "OVER TEMP       ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}

		if(CLOCK.T_mS > 75 && CLOCK.T_mS < 100)
		{
			if(MPPT2.NOC)
			{
				sprintf(buffer, "NO BATTERY      ");
				lcd_putstring(3,0, buffer);
			}

			else
			{
				sprintf(buffer, "                ");
				lcd_putstring(3,0, buffer);
			}
		}
	}

	else
	{
		// Reset Power Variables if not connected
		sprintf(buffer, "                    ");
		lcd_putstring(1,0, buffer);
		sprintf(buffer, "                    ");
		lcd_putstring(3,0, buffer);

		// flash every 1 second if not connected
		if(((CLOCK.T_mS > 0 && CLOCK.T_mS < 25) || (CLOCK.T_mS > 50 && CLOCK.T_mS < 75)))
		{
			sprintf(buffer, "**CONNECTION ERROR**");
			lcd_putstring(2,0, buffer);
		}

		else
		{
			sprintf(buffer, "                    ");
			lcd_putstring(2,0, buffer);
		}
	}
}


/******************************************************************************
** Function name:		lcd_display_MPPTPower
**
** Description:			Total power from MPPTs
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_MPPTPower (void) // menus[7]
{
	char buffer[20];

	sprintf(buffer, "-POWER IN-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "MPPT1: %.2f Whrs  ", MPPT1.WattHrs);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "MPPT2: %.2f Whrs  ", MPPT2.WattHrs);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "TOTAL: %.2f Whrs  ", MPPT1.WattHrs + MPPT2.WattHrs);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		MPPT1.WattHrs = 0;
		MPPT2.WattHrs = 0;
		buzzer(300);
	}
}


/******************************************************************************
** Function name:		lcd_display_motor
**
** Description:			Motor stats screens
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_motor (void) // menus[8]
{
	char buffer[20];

	switch(MENU.SUBMENU_POS){
	default:
		MENU.SUBMENU_POS = 0;
		/* no break */
	case 0:
		sprintf(buffer, "-MTR PWR-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "ESC: %.1fV @ %.1fA ", ESC.Bus_V, ESC.Bus_I);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "Total Power: %.1f ",  ESC.Watts);
		lcd_putstring(3,0, buffer);
		break;
	case 1:
		sprintf(buffer, "-PWR USED-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "ESC: %.2f W/hrs", ESC.WattHrs);
		lcd_putstring(1,0, buffer);

		break;
	case 2: // TODO: Display peak V?
		sprintf(buffer, "-MTR PKS-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "%.1fAp @ %.1fWp ", ESC.MAX_Bus_I, ESC.MAX_Watts);
		lcd_putstring(1,0, buffer);


		sprintf(buffer, "                    ");
		lcd_putstring(3,0, buffer);
		if (SELECT)
		{
			ESC.MAX_Bus_I = ESC.MAX_Watts = 0;
			ESC.MAX_Bus_V = 0;

			buzzer(600);
		}
		break;
	}

	if(INCREMENT){MENU.SUBMENU_POS++;}
	else if(DECREMENT){MENU.SUBMENU_POS--;}
}


/******************************************************************************
** Function name:		lcd_display_debug
**
** Description:			Bus debug screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_debug (void) // menus[9]
{// TODO: Update field names
	char buffer[20];

	sprintf(buffer, "-DEBUG-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "BUS E: %.1f Whrs ", BMU.WattHrs);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "BUS I: %.1f Amps ", BMU.Battery_I);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "BUS P: %.1f Watts", BMU.Watts);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		BMU.WattHrs = 0;
		buzzer(300);
	}
}


/******************************************************************************
** Function name:		lcd_display_errors
**
** Description:			Error display screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_errors (void) // menus[10]
{ // TODO: Show all errors?
	char buffer[20];

	sprintf(buffer, "-ESC FALT-");
	_lcd_putTitle(buffer);

	// If no ERRORS then display MPPT Watts
	if(ESC1.ERROR && ESC2.ERROR)
	{
		sprintf(buffer, "ESC1 & ESC2 FAULTS  ");
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "CODE: %d ", ESC.ERROR);
		lcd_putstring(3,0, buffer);
	}

	else if(ESC.ERROR)
	{
		sprintf(buffer, "ESC FAULT          ");
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "CODE: %d", ESC1.ERROR);
		lcd_putstring(3,0, buffer);
	}

	else
	{
		sprintf(buffer, "NO FAULTS           ");
		lcd_putstring(2,0, buffer);
	}


			//////////////////////////////////////////////////////////////
			//////////////////   Fix for motor reset   ///////////////////
			//////////////////////////////////////////////////////////////

	// Don't reset on HWOC
	if(SELECT && ((ESC1.ERROR & 0x1) || (ESC2.ERROR & 0x1)))	// MOTOR CONTROLLER ERROR RESET	GOES BELOW	--	NOT YET TESTED
	{
		sprintf(buffer, "RESET MOTOR CONTROLS");
		lcd_putstring(0,0, buffer);
		lcd_putstring(1,0, buffer);
		lcd_putstring(2,0, buffer);
		lcd_putstring(3,0, buffer);
		buzzer(500);

		// RESET MOTOR CONTROLLERS
		// see WS22 user manual and Tritium CAN network specs
		// try MC + 25 (0x19) + msg "RESETWS" (TRI88.004 ver3 doc, July 2013)

		if((LPC_CAN1->GSR & (1 << 3)))				// If previous transmission is complete, send message;
		{
			MsgBuf_TX1.Frame = 0x00080000; 			/* 11-bit, no RTR, DLC is 8 bytes */
			MsgBuf_TX1.MsgID = 0x503; 		/* Explicit Standard ID */
			MsgBuf_TX1.DataB = 0x0;
			MsgBuf_TX1.DataA = 0x0;
			CAN1_SendMessage( &MsgBuf_TX1 );
			buzzer(20);
		}
	}
}


/******************************************************************************
** Function name:		lcd_display_other
**
** Description:			Other options on this screen.
** 						Buzzer and backlight settings
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_other (void) // menus[11]
{
	// TODO: Update DUTYBL here instead of drive routine
	char buffer[20];

	sprintf(buffer, "-OTHER-");
	_lcd_putTitle(buffer);

	if (MENU.SELECTED || (CLOCK.T_S%2))
	{
		switch(MENU.ITEM_SELECTOR)
		{
		default:
			MENU.ITEM_SELECTOR = 0;
			/* no break */
		case 0:
			if(STATS.BUZZER){
				sprintf(buffer, ">> STATS.BUZZER:        ON");
				lcd_putstring(2,0, buffer);}
			else{
				sprintf(buffer, ">> STATS.BUZZER:       OFF");
				lcd_putstring(2,0, buffer);}
			sprintf(buffer, "   BACKLIGHT:    %03d", PWMBL/10);
			lcd_putstring(3,0, buffer);
			break;
		case 1:
			if(STATS.BUZZER){
				sprintf(buffer, "   STATS.BUZZER:        ON");
				lcd_putstring(2,0, buffer);}
			else{
				sprintf(buffer, "   STATS.BUZZER:       OFF");
				lcd_putstring(2,0, buffer);}
			sprintf(buffer, ">> BACKLIGHT:    %03d", PWMBL/10);
			lcd_putstring(3,0, buffer);
			break;
		}
	}
	else
	{
		if(STATS.BUZZER){
			sprintf(buffer, "   STATS.BUZZER:        ON");
			lcd_putstring(2,0, buffer);}
		else{
			sprintf(buffer, "   STATS.BUZZER:       OFF");
			lcd_putstring(2,0, buffer);}
		sprintf(buffer, "   BACKLIGHT:    %03d", PWMBL/10);
		lcd_putstring(3,0, buffer);
	}
	////////////////////////////////////////////////////////////////////////

	/////////////////////////////   ACTIONS   //////////////////////////////
	if(MENU.SELECTED)
	{
		if(SELECT)
		{
			MENU.SELECTED = 0;
			EEWrite(AddressBUZZ, STATS.BUZZER);
		}
		else if(INCREMENT)
		{
			switch(MENU.ITEM_SELECTOR)
			{
			case 0:
				if(STATS.BUZZER){BUZZER_OFF}
				else{BUZZER_ON}
				break;
			case 1:
				if(PWMBL > 1000){PWMBL += 10;}
				else if (PWMBL >= 1000){PWMBL = 1000;buzzer(200);}
				break;
			default:
				break;
			}
		}
		else if(DECREMENT)
		{
			switch(MENU.ITEM_SELECTOR)
			{
			case 0:
				if(STATS.BUZZER){BUZZER_OFF}
				else{BUZZER_ON}
				break;
			case 1:
				if(PWMBL > 0){PWMBL -= 10;}
				else if(PWMBL <= 0){PWMBL = 0;buzzer(200);}
				break;
			default:
				break;
			}
		}
	}
	else
	{
		if(SELECT){MENU.SELECTED = 1;}
		else if(INCREMENT)
		{
			MENU.ITEM_SELECTOR++;
			if(MENU.ITEM_SELECTOR > 1)
			{
				MENU.ITEM_SELECTOR = 0;
			}
		}
		else if(DECREMENT)
		{
			MENU.ITEM_SELECTOR--;
			if(MENU.ITEM_SELECTOR < 0)
			{
				MENU.ITEM_SELECTOR = 1;
			}
		}
	}
}


/******************************************************************************
** Function name:		lcd_display_peaks
**
** Description:			Car peaks screen
** 						1. Array power
** 						2. Top speed
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_peaks (void) // menus[12]
{
	char buffer[20];

	sprintf(buffer, "-DATA PKS-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "ARRAY: %.1f Watts  ", MPPT1.MAX_Watts + MPPT2.MAX_Watts);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "TOP SPD: %3.1f kmh  ", STATS.MAX_SPEED);
	lcd_putstring(3,0, buffer);

	if(SELECT)
	{
		MPPT1.MAX_Watts = 0;
		MPPT2.MAX_Watts = 0;
		STATS.MAX_SPEED = 0;
		buzzer(300);
	}
}


/******************************************************************************
** Function name:		lcd_display_runtime
**
** Description:			Displays car's current runtime
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_runtime (void) // menus[13]
{
	char buffer[20];

	sprintf(buffer, "-RUNTIME-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "%luD%02dhr", CLOCK.T_D, CLOCK.T_H);
	lcd_putstring(2,14, buffer);

	sprintf(buffer, "%02dm%02d.%01ds", CLOCK.T_M, CLOCK.T_S, CLOCK.T_mS/10);
	lcd_putstring(3,12, buffer);
}

/******************************************************************************
** Function name:		lcd_display_odometer
**
** Description:			Displays odometer
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_odometer (void) // menus[14]
{
	char buffer[20];

	sprintf(buffer, "-ODOMETER-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "CAR: %.5f KM   ", STATS.ODOMETER);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "ESC: %.5f KM   ", ESC.Odometer); //STATS.ODOMETER_REV);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		STATS.ODOMETER = 0;
		//STATS.ODOMETER_REV = 0;
		buzzer(300);
	}
}
///////////////////////////////////////////////

///////////////////////////////////////////////
/// errors array

/******************************************************************************
** Function name:		lcd_display_SWOC
**
** Description:			Display screen for SWOC error
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_SWOC (void) // errors[0]
{
	char buffer[20];

	sprintf(buffer, "-SWOC ERR-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "*******ERROR!*******");
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "PRESS SELECT 2 RESET");
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "PRESS OTHER 2 CANCEL");
	lcd_putstring(3,0, buffer);

	// BUTTONS
	if(SELECT)
	{
		if((LPC_CAN1->GSR & (1 << 3)))				// If previous transmission is complete, send message;
		{
			MsgBuf_TX1.Frame = 0x00080000; 			/* 11-bit, no RTR, DLC is 8 bytes */
			MsgBuf_TX1.MsgID = 0x503; 		/* Explicit Standard ID */
			MsgBuf_TX1.DataB = 0x0;
			MsgBuf_TX1.DataA = 0x0;
			CAN1_SendMessage( &MsgBuf_TX1 );
			buzzer(20);
		}
	}
	else if(INCREMENT || DECREMENT)
	{
		// mark error acknowledged
		STATS.SWOC_ACK = TRUE;
	}
}

/******************************************************************************
** Function name:		lcd_display_HWOC
**
** Description:			Display screen for HWOC error
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_display_HWOC (void) // errors[1]
{
	char buffer[20];

	sprintf(buffer, "-HWOC ERR-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "*******ERROR!*******");
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "PRESS SELECT 2 RESET");
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "PRESS OTHER 2 CANCEL");
	lcd_putstring(3,0, buffer);

	BUZZER_ON

	// BUTTONS
	if(SELECT)
	{
		if((LPC_CAN1->GSR & (1 << 3)))				// If previous transmission is complete, send message;
		{
			MsgBuf_TX1.Frame = 0x00080000; 			/* 11-bit, no RTR, DLC is 8 bytes */
			MsgBuf_TX1.MsgID = 0x503; 		/* Explicit Standard ID */
			MsgBuf_TX1.DataB = 0x0;
			MsgBuf_TX1.DataA = 0x0;
			CAN1_SendMessage( &MsgBuf_TX1 );
			buzzer(20);
		}
	}
	else if(INCREMENT || DECREMENT){STATS.HWOC_ACK = TRUE;}// mark error acknowledged
}
///////////////////////////////////////////////

/******************************************************************************
** Function name:		_lcd_putTitle
**
** Description:			Used place the screen title and current car speed on
** 						top line of LCD
**
** Parameters:			1. Address of char array with title string (10 character max)
** Returned value:		None
**
******************************************************************************/
void _lcd_putTitle (char *_title)
{
	// displays first 10 chars of _title and current speed at (0,0) on lcd screen
	char buffer[20];
	char spd[11];
	char *bufadd;
	char *spdadd;

	bufadd = buffer;
	spdadd = spd;

	sprintf(buffer, _title);
	while (*(++bufadd) != '\0') // TODO: Check logic for _title with more than 10 char
	{;}

	for (;bufadd != buffer + 10; bufadd++)
	{*bufadd = ' ';}

	sprintf(spd, " %05.1fkmh ", ESC.Velocity_KMH);

	for (;bufadd != buffer + 20; bufadd++)
	{
		*bufadd = *spdadd;
		spdadd++;
	}

	lcd_putstring(0, 0, buffer);
}

/******************************************************************************
** Function name:		menuInit
**
** Description:			Initialize menu arrays
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void menuInit (void)
{
	MENU.errors[0] = lcd_display_SWOC;
	MENU.errors[1] = lcd_display_HWOC;

	MENU.menus[0] = lcd_display_info;
	MENU.menus[1] = lcd_display_escBus;
	MENU.menus[2] = lcd_display_home;
	MENU.menus[3] = lcd_display_drive;
	MENU.menus[4] = lcd_display_cruise;
	MENU.menus[5] = lcd_display_MPPT1;
	MENU.menus[6] = lcd_display_MPPT2;
	MENU.menus[7] = lcd_display_MPPTPower;
	MENU.menus[8] = lcd_display_motor;
	MENU.menus[9] = lcd_display_debug;
	MENU.menus[10] = lcd_display_errors;
	MENU.menus[11] = lcd_display_other;
	MENU.menus[12] = lcd_display_peaks;
	MENU.menus[13] = lcd_display_runtime;
	MENU.menus[14] = lcd_display_odometer;

	MENU.MENU_POS = 1;
}

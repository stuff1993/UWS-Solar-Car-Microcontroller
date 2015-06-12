/*
 * menu.c
 *
 *  Created on: 29 May 2015
 *      Author: Stuff
 */

#include <stdio.h>
#include <stdint.h>
#include "menu.h"

#include "lpc17xx.h"

extern MOTORCONTROLLER ESC1, ESC2;
extern MPPT MPPT1, MPPT2;
extern CAN_MSG MsgBuf_TX1;
extern volatile uint16_t PWMA, PWMC; // TODO: remove
extern uint16_t PWMBL;
extern uint16_t THR_POS, RGN_POS;


//////////////////////////////////////////////
/// Not in array, reference manually

/******************************************************************************
** Function name:		displayERRORonSTART
**
** Description:			Error screen on boot
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayERRORonSTART (void)
{
	lcd_putstring(0,0, "--    CAUTION!    --");
	lcd_putstring(1,0, "                    ");
	lcd_putstring(2,0, "   GEARS ENGAGED!   ");
	lcd_putstring(3,0, "                    ");
}

/******************************************************************************
** Function name:		displayINTRO
**
** Description:			Boot intro screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayINTRO (void)
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
	delayMs(1,100);
	BUZZER_OFF;

	lcdCLEAR();
}
///////////////////////////////////////////////

///////////////////////////////////////////////
/// menus array

/******************************************************************************
** Function name:		displayINFO
**
** Description:			Car information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayINFO (void) // menus[0]
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
** Function name:		displayBUSDATA
**
** Description:			Data screen for precharge
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayBUSDATA (void) // menus[1]
{// TODO: CHANGE TO CONTROL PRECHARGE
	char buffer[20];

	sprintf(buffer, "-ESC BUS-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "BUS VOLTAGE: %03.0f V  ", ESC1.BUS_VOLTAGE);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "BAT VOLTAGE: %03lu V  ", BMU1.BATTERY_CURRENT);
	lcd_putstring(2,0, buffer);

	if((BMU1.BATTERY_VOLTAGE - ESC1.BUS_VOLTAGE) < 10)
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
** Function name:		displayHOME
**
** Description:			Speed, drive, array power, basic errors
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayHOME (void) // menus[2]
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

	if(CRUISE.ACTIVE)
	{
		if (CRUISE.CO >= 1000){sprintf(buffer, "CRUISE FW: %3d.%d%%   ", PWMA/10,PWMA%10);}
		else{sprintf(buffer, "CRUISE RG: %3d.%d%%   ", PWMC/10,PWMC%10);}

		lcd_putstring(2,0, buffer);
	}
	else if(FORWARD && !RGN_POS)
	{
		sprintf(buffer, "DRIVE:     %3d.%d%%   ", PWMA/10,PWMA%10);
		lcd_putstring(2,0, buffer);
	}

	else if(REVERSE && !RGN_POS){
	sprintf(buffer, "REVERSE:   %3d.%d%%   ", PWMA/10,PWMA%10);
	lcd_putstring(2,0, buffer);}

	else if(RGN_POS){
	sprintf(buffer, "REGEN:     %3d.%d%%   ", PWMC/10,PWMC%10);
	lcd_putstring(2,0, buffer);	}

	else{
	sprintf(buffer, "NEUTRAL:   %3d.%d%%   ", PWMA/10,PWMA%10);
	lcd_putstring(2,0, buffer);}



	// If no ERRORS then display MPPT Watts
	// TODO: ??? check all errors?
	if(ESC1.ERROR && ESC2.ERROR)
	{
		sprintf(buffer, "ESC1 & ESC2 FAULTS  ");
		lcd_putstring(3,0, buffer);
	}

	else if(ESC1.ERROR)
	{
		sprintf(buffer, "ESC1 FAULT          ");
		lcd_putstring(3,0, buffer);
	}

	else if(ESC2.ERROR)
	{
		sprintf(buffer, "ESC2 FAULT          ");
		lcd_putstring(3,0, buffer);
	}

	else
	{
		sprintf(buffer, "ARRAY:    %3.1f W  ", MPPT2.WATTS+MPPT1.WATTS);
		lcd_putstring(3,0, buffer);
	}
}

/******************************************************************************
** Function name:		displayDRIVE
**
** Description:			Drive details screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayDRIVE (void) // menus[3]
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
		sprintf(buffer, "OUTPUT:       %3d.%d%%", PWMA/10,PWMA%10);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "THROTTLE:     %3d.%d%%", THR_POS/10,THR_POS%10);
		lcd_putstring(3,0, buffer);
	}

	else
	{
		sprintf(buffer, "OUTPUT:       %3d.%d%%", PWMC/10,PWMC%10);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "REGEN:        %3d.%d%%", RGN_POS/10,RGN_POS%10);
		lcd_putstring(3,0, buffer);
	}
}

/******************************************************************************
** Function name:		displayCRUISE
**
** Description:			Cruise control screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayCRUISE (void) // menus[4]
{// TODO: MAJOR REWORK FOR WS CRUISE
	char buffer[20];

	if (MENU.SUBMENU_POS == 0) // Cruise Menu
	{
		sprintf(buffer, "-CRUISE-");
		_lcd_putTitle(buffer);
		if (!REVERSE)
		{
			if (CRUISE.STATUS && CRUISE.ACTIVE)
			{
				sprintf(buffer, " STS:  ON  ACT:  ON ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET: %3.0f  THR:%3d%% ", CRUISE.SP, PWMA/10);
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:%3d%% ", ESC1.VELOCITY_KMHR, PWMC/10);
				lcd_putstring(3,0, buffer);

				// Button presses

				if (SELECT)
				{
					// ACT: OFF
					CRUISE.ACTIVE = 0;
				}
				else if (INCREMENT)
				{
					// SET + 1
					CRUISE.SP += 1;
				}
				else if (DECREMENT)
				{
					// SET - 1
					CRUISE.SP -= 1;
				}
			}
			else if (CRUISE.STATUS && !CRUISE.ACTIVE)
			{
				sprintf(buffer, " STS:  ON  ACT: OFF ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET: %3.0f  THR:     ", CRUISE.SP);
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:     ", ESC1.VELOCITY_KMHR);
				lcd_putstring(3,0, buffer);

				// Button presses

				if (SELECT)
				{
					// STS: OFF
					CRUISE.STATUS = 0;
					CRUISE.SP = 0;
					CRUISE.CO = 0;
				}
				else if (INCREMENT)
				{
					// check spd set
					// ACT: ON
					if (CRUISE.SP > 1)
					{
						CRUISE.CO = PWMA + 1000;
						CRUISE.ACTIVE = 1;
					}
				}
				else if (DECREMENT)
				{
					// SET: SPD
					// ACT: ON
					CRUISE.SP = ESC1.VELOCITY_KMHR;
					CRUISE.CO = PWMA + 1000;
					CRUISE.ACTIVE = 1;
				}
			}
			else if (CRUISE.ACTIVE && !CRUISE.STATUS) // Should never trip, but just in case
			{
				sprintf(buffer, " STS: OFF  ACT:  ON ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, "    CRUISE ERROR    ");
				lcd_putstring(2,0, buffer);

				sprintf(buffer, "     RESETTING      ");
				lcd_putstring(3,0, buffer);

				CRUISE.ACTIVE = 0;
				CRUISE.STATUS = 0;
				CRUISE.SP = 0;
			}
			else
			{
				sprintf(buffer, " STS: OFF  ACT: OFF ");
				lcd_putstring(1,0, buffer);

				sprintf(buffer, " SET:      THR:     ");
				lcd_putstring(2,0, buffer);

				sprintf(buffer, " SPD: %3.0f  RGN:     ", ESC1.VELOCITY_KMHR);
				lcd_putstring(3,0, buffer);

				// Button presses

				if (SELECT)
				{
					// STS: ON
					CRUISE.SP = 0;
					CRUISE.STATUS = 1;
				}
				else if (INCREMENT)
				{
					MENU.SUBMENU_POS = 1;
				}
				else if (DECREMENT)
				{
					MENU.SUBMENU_POS = 2;
				}
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
			CRUISE.STATUS = 0;
			CRUISE.ACTIVE = 0;
			CRUISE.CO = 0;
			CRUISE.SP = 0;
		}
	}
	else if (MENU.SUBMENU_POS == 1) // PID Menu
	{
		sprintf(buffer, "-CRU PI-");
		_lcd_putTitle(buffer);
		if (MENU.SELECTED || (CLOCK.T_S % 2))
		{
			switch (MENU.ITEM_SELECTOR)
			{
				default:
					MENU.ITEM_SELECTOR = 0;
					/* no break */
				case 0:
					sprintf(buffer, " Kc: %3.1f <<   RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f      SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T: %3.1f       EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;

				case 1:
					sprintf(buffer, " Kc: %3.1f      RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f <<   SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T:  %3.1f      EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;

				case 2:
					sprintf(buffer, " Kc: %3.1f      RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f      SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T:  %3.1f <<   EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;

				case 3:
					sprintf(buffer, " Kc: %3.1f   >> RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f      SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T:  %3.1f      EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;

				case 4:
					sprintf(buffer, " Kc: %3.1f      RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f   >> SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T:  %3.1f      EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;

				case 5:
					sprintf(buffer, " Kc: %3.1f      RESET ", CRUISE.Kc);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, " Ti: %3.1f      SAVE  ", CRUISE.Ti);
					lcd_putstring(2,0, buffer);

					sprintf(buffer, " T:  %3.1f   >> EXIT  ", CRUISE.T);
					lcd_putstring(3,0, buffer);
					break;
			}
		}
		else
		{
			sprintf(buffer, " Kc: %3.1f      RESET ", CRUISE.Kc);
			lcd_putstring(1,0, buffer);

			sprintf(buffer, " Ti: %3.1f      SAVE  ", CRUISE.Ti);
			lcd_putstring(2,0, buffer);

			sprintf(buffer, " T:  %3.1f      EXIT  ", CRUISE.T);
			lcd_putstring(3,0, buffer);
		}

		if (MENU.SELECTED) // Button checks
		{
			if (SELECT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 5: // reset & exit to cruise menu
						MENU.SUBMENU_POS = 0;
						MENU.ITEM_SELECTOR = 0;
						/* no break */
					case 3: // reset (read from eeprom)
						CRUISE.Kc = convertToFloat(EERead(AddressCRU_Kc));
						CRUISE.Ti = convertToFloat(EERead(AddressCRU_Ti));
						CRUISE.T = convertToFloat(EERead(AddressCRU_T));
						break;
					case 4: // save to eeprom
						EEWrite(AddressCRU_Kc, convertToUint(CRUISE.Kc));
						delayMs(1,3);
						EEWrite(AddressCRU_Ti, convertToUint(CRUISE.Ti));
						delayMs(1,3);
						EEWrite(AddressCRU_T, convertToUint(CRUISE.T));
						delayMs(1,3);
						break;
				}
				MENU.SELECTED = 0;
			}
			else if (INCREMENT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 0:
						// inc Controller Gain
						CRUISE.Kc += 0; // edit value
						break;
					case 1:
						// inc Reset Time
						CRUISE.Ti += 0; // edit value
						break;
					case 2:
						// inc Sample Time
						CRUISE.T += 0; // edit value
						break;
					default:
					case 3:
					case 4:
					case 5:
						MENU.SELECTED = 0;
						break;
				}
			}
			else if (DECREMENT)
			{
				switch (MENU.ITEM_SELECTOR)
				{
					case 0:
						// dec Controller Gain
						CRUISE.Kc -= 0; // edit value
						break;
					case 1:
						// dec Reset Time
						CRUISE.Ti -= 0; // edit value
						break;
					case 2:
						// dec Sample Time
						CRUISE.T -= 0; // edit value
						break;
					default:
					case 3:
					case 4:
					case 5:
						MENU.SELECTED = 0;
						break;
				}
			}
		}
		else
		{
			if (SELECT)
			{
				MENU.SELECTED = 1;
			}
			else if (INCREMENT)
			{
				MENU.ITEM_SELECTOR++;
				if(MENU.ITEM_SELECTOR > 5){MENU.ITEM_SELECTOR = 0;}
			}
			else if (DECREMENT)
			{
				MENU.ITEM_SELECTOR--;
				if(MENU.ITEM_SELECTOR < 0){MENU.ITEM_SELECTOR = 5;}
			}
		}
	}
	else if (MENU.SUBMENU_POS == 2)
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
					sprintf(buffer, "CO:%04.0f <<  SPD:%5.1f ", CRUISE.SP, ESC1.VELOCITY_KMHR);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET                 ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET           EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 1:
					sprintf(buffer, "CO:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC1.VELOCITY_KMHR);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET     <<          ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET           EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 2:
					sprintf(buffer, "CO:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC1.VELOCITY_KMHR);
					lcd_putstring(1,0, buffer);

					sprintf(buffer, "SET                 ");
					lcd_putstring(2,0, buffer);

					sprintf(buffer, "RESET   <<      EXIT");
					lcd_putstring(3,0, buffer);
					break;

				case 3:
					sprintf(buffer, "CO:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC1.VELOCITY_KMHR);
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
			sprintf(buffer, "CO:%04.0f     SPD:%5.1f ", CRUISE.SP, ESC1.VELOCITY_KMHR);
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
** Function name:		displayMPPT1
**
** Description:			MPPT1 information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayMPPT1(void) // menus[5]
{
	char buffer[20];

	sprintf(buffer, "-MPPT 1-");
	_lcd_putTitle(buffer);

	if(MPPT1.CONNECTED)
	{
		sprintf(buffer, "IN: %3lu.%luV @ %2lu.%02luA ", MPPT1.VOLTS_INPUT/10, MPPT1.VOLTS_INPUT%10, MPPT1.CURRENT_IN/100, MPPT1.CURRENT_IN%100);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "OUT:%3lu.%luV @ %3.1fW ", MPPT1.VOLTS_OUTPUT/10, MPPT1.VOLTS_OUTPUT%10, MPPT1.WATTS);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "%2lu%cC", MPPT1.TEMP_DEGREES, 0xDF);
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
** Function name:		displayMPPT2
**
** Description:			MPPT2 information screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayMPPT2(void) // menus[6]
{
	char buffer[20];

	sprintf(buffer, "-MPPT 2-");
	_lcd_putTitle(buffer);

	if(MPPT2.CONNECTED)
	{
		sprintf(buffer, "IN: %3lu.%luV @ %2lu.%02luA ", MPPT2.VOLTS_INPUT/10, MPPT2.VOLTS_INPUT%10, MPPT2.CURRENT_IN/100, MPPT2.CURRENT_IN%100);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "OUT:%3lu.%luV @ %3.1fW ", MPPT2.VOLTS_OUTPUT/10, MPPT2.VOLTS_OUTPUT%10, MPPT2.WATTS);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "%2lu%cC", MPPT2.TEMP_DEGREES, 0xDF);
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
** Function name:		displayMPPT_MAH
**
** Description:			Total power from MPPTs
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayMPPT_MAH (void) // menus[7]
{
	char buffer[20];

	sprintf(buffer, "-POWER IN-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "MPPT1: %.2f Whrs  ", MPPT1.WATTHRS);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "MPPT2: %.2f Whrs  ", MPPT2.WATTHRS);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "TOTAL: %.2f Whrs  ", STATS.MPPT_WHR_TOTAL);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		MPPT1.WATTHRS = 0;
		MPPT2.WATTHRS = 0;
		buzzer(300);
	}
}

/******************************************************************************
** Function name:		displayMOTOR
**
** Description:			Motor stats screens
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayMOTOR (void) // menus[8]
{
	char buffer[20];

	switch(MENU.SUBMENU_POS){
	default:
		MENU.SUBMENU_POS = 0;
		/* no break */
	case 0:
		sprintf(buffer, "-MTR PWR-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "ESC1: %.1fV @ %.1fA ", ESC1.BUS_VOLTAGE, ESC1.BUS_CURRENT);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "ESC2: %.1fV @ %.1fA ", ESC2.BUS_VOLTAGE, ESC2.BUS_CURRENT);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "Total Power: %.1f ",  ESC1.WATTS + ESC2.WATTS);
		lcd_putstring(3,0, buffer);
		break;
	case 1:
		sprintf(buffer, "-PWR USED-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "ESC1: %.2f W/hrs", ESC1.WATT_HOUR_USED);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "ESC2: %.2f W/hrs", ESC2.WATT_HOUR_USED);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "TOTAL: %.2f W/hrs", STATS.ESC_WHR_TOTAL);
		lcd_putstring(3,0, buffer);
		break;
	case 2:
		sprintf(buffer, "-MTR PKS-");
		_lcd_putTitle(buffer);

		sprintf(buffer, "%.1fAp @ %.1fWp ", ESC1.PEAK_CURR, ESC1.PEAKWATTS);
		lcd_putstring(1,0, buffer);

		sprintf(buffer, "%.1fAp @ %.1fWp ", ESC2.PEAK_CURR, ESC2.PEAKWATTS);
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "                    ");
		lcd_putstring(3,0, buffer);
		if (SELECT)
		{
			ESC1.PEAK_CURR = ESC1.PEAKWATTS = 0;
			ESC2.PEAK_CURR = ESC2.PEAKWATTS = 0;

			buzzer(600);
		}
		break;
	}

	if(INCREMENT){MENU.SUBMENU_POS++;}
	else if(DECREMENT){MENU.SUBMENU_POS--;}
}

/******************************************************************************
** Function name:		displayDEBUG
**
** Description:			Bus debug screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayDEBUG (void) // menus[9]
{
	char buffer[20];

	sprintf(buffer, "-DEBUG-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "BUS E: %.1f Whrs ", BMU1.WHR_TOTAL);
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "BUS I: %.1f Amps ", BMU1.BUS_CURRENT);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "BUS P: %.1f Watts", BMU1.WATTS);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		BMU1.WHR_TOTAL = 0;
		buzzer(300);
	}
}

/******************************************************************************
** Function name:		displayERRORS
**
** Description:			Error display screen
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayERRORS (void) // menus[10]
{
	char buffer[20];

	sprintf(buffer, "-ESC FALT-");
	_lcd_putTitle(buffer);

	// If no ERRORS then display MPPT Watts
	if(ESC1.ERROR && ESC2.ERROR)
	{
		sprintf(buffer, "ESC1 & ESC2 FAULTS  ");
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "CODE: %d - %d", ESC1.ERROR, ESC2.ERROR);
		lcd_putstring(3,0, buffer);
	}

	else if(ESC1.ERROR)
	{
		sprintf(buffer, "ESC1 FAULT          ");
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "CODE: %d", ESC1.ERROR);
		lcd_putstring(3,0, buffer);
	}

	else if(ESC2.ERROR)
	{
		sprintf(buffer, "ESC2 FAULT          ");
		lcd_putstring(2,0, buffer);

		sprintf(buffer, "CODE: %d", ESC2.ERROR);
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
** Function name:		displayOTHER
**
** Description:			Other options on this screen.
** 						Buzzer and backlight settings
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayOTHER (void) // menus[11]
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
** Function name:		displayPEAKS
**
** Description:			Car peaks screen
** 						1. Array power
** 						2. Top speed
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayPEAKS (void) // menus[12]
{
	char buffer[20];

	sprintf(buffer, "-DATA PKS-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "ARRAY: %.1f Watts  ", MPPT1.WATTSPEAK + MPPT2.WATTSPEAK);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "TOP SPD: %3.1f kmh  ", STATS.maxSPEED);
	lcd_putstring(3,0, buffer);

	if(SELECT)
	{
		MPPT1.WATTSPEAK = 0;
		MPPT2.WATTSPEAK = 0;
		STATS.maxSPEED = 0;
		buzzer(300);
	}
}

/******************************************************************************
** Function name:		displayRUNTIME
**
** Description:			Displays car's current runtime
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayRUNTIME (void) // menus[13]
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
** Function name:		displayODOMETER
**
** Description:			Display's odometer
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayODOMETER (void) // menus[14]
{// TODO: ESC ODO
	char buffer[20];

	sprintf(buffer, "-ODOMETER-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "FWD: %.5f KM   ", STATS.ODOMETER);
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "REV: %.5f KM   ", STATS.ODOMETER_REV);
	lcd_putstring(3,0, buffer);

	if(SELECT && INCREMENT)
	{
		STATS.ODOMETER = 0;
		STATS.ODOMETER_REV = 0;
		buzzer(300);
	}
}
///////////////////////////////////////////////

///////////////////////////////////////////////
/// errors array

/******************************************************************************
** Function name:		displaySWOC
**
** Description:			Display screen for SWOC error
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displaySWOC (void) // errors[0]
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
** Function name:		displayHWOC
**
** Description:			Display screen for HWOC error
**
** Parameters:			None
** Returned value:		None
**
******************************************************************************/
void displayHWOC (void) // errors[1]
{// TODO: ADD RESET??
	char buffer[20];

	sprintf(buffer, "-HWOC ERR-");
	_lcd_putTitle(buffer);

	sprintf(buffer, "*******ERROR!*******");
	lcd_putstring(1,0, buffer);

	sprintf(buffer, "HWOC REQUIRE PWR OFF");
	lcd_putstring(2,0, buffer);

	sprintf(buffer, "PRESS OTHER 2 CANCEL");
	lcd_putstring(3,0, buffer);

	BUZZER_ON

	// BUTTONS
	if(INCREMENT || DECREMENT || SELECT)
	{
		// mark error acknowledged
		STATS.HWOC_ACK = TRUE;
		BUZZER_OFF
	}
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
	while (*(++bufadd) != '\0')
	{;}

	for (;bufadd != buffer + 10; bufadd++)
	{*bufadd = ' ';}

	sprintf(spd, " %05.1fkmh ", ESC1.VELOCITY_KMHR);

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
	MENU.errors[0] = SWOC;
	MENU.errors[1] = HWOC;

	MENU.menus[0] = displayINFO;
	MENU.menus[1] = displayBUSDATA;
	MENU.menus[2] = displayHOME;
	MENU.menus[3] = displayDRIVE;
	MENU.menus[4] = displayCRUISE;
	MENU.menus[5] = displayMPPT1;
	MENU.menus[6] = displatMPPT2;
	MENU.menus[7] = displayMPPT_MAH;
	MENU.menus[8] = displayMOTOR;
	MENU.menus[9] = displayDEBUG;
	MENU.menus[10] = displayERRORS;
	MENU.menus[11] = displayOTHER;
	MENU.menus[12] = displayPEAKS;
	MENU.menus[13] = displayRUNTIME;
	MENU.menus[14] = displayODOMETER;

	MENU.MENU_POS = 1;
}

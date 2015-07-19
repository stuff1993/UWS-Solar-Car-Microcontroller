/****************************************************************************
 *  pwm.c
****************************************************************************/
#include "lpc17xx.h"
#include "type.h"
#include "pwm.h"

volatile uint32_t match_counter0, match_counter1;

/******************************************************************************
** Function name:		PWM1_IRQHandler
**
** Descriptions:		PWM1 interrupt handler
**						For now, it only deals with PWM1 match 0
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void PWM1_IRQHandler (void) 
{
  uint32_t regVal;

  regVal = LPC_PWM1->IR;
  if ( regVal & MR0_INT )
  {
	match_counter1++;	
  }
  LPC_PWM1->IR |= regVal;		/* clear interrupt flag on match 0 */
  return;
}

/******************************************************************************
** Function name:		PWM_Init
**
** Descriptions:		PWM initialization, setup all GPIOs to PWM0~6,
**						reset counter, all latches are enabled, interrupt
**						on PWMMR0, install PWM interrupt to the VIC table.
**
** parameters:			ChannelNum, Duty cycle
** Returned value:		true or fase, if VIC table is full, return false
** 
******************************************************************************/
uint32_t PWM_Init( uint32_t ChannelNum, uint32_t cycle )
{
    LPC_SC->PCONP 	|= (1<<6);
	LPC_SC->PCLKSEL0  |= (1<<12);		// enable PWM Clock;
	LPC_PWM1->MCR = (1<<0);		/* reset on match  */


	if ( ChannelNum == 1 )
	{
		match_counter1 = 0;
		LPC_PINCON->PINSEL4 |= 0b010000010101;	/* set GPIOs for all PWM pins on PWM */

		LPC_PWM1->TCR = TCR_RESET;	/* Counter Reset */
		LPC_PWM1->PR = 0x00;		/* count frequency:Fpclk */


		LPC_PWM1->MR0 = cycle;		/* set PWM cycle */
		LPC_PWM1->MR1 = cycle * 1/2;
		LPC_PWM1->MR2 = cycle * 1/2;
		LPC_PWM1->MR3 = cycle * 1/2;
		LPC_PWM1->MR6 = cycle * 1/2;

		/* all PWM latch enabled */
		LPC_PWM1->LER = LER0_EN | LER1_EN | LER2_EN | LER3_EN | LER6_EN;
	}

	return (TRUE);
}


/******************************************************************************
** Function name:		PWM_Start
**
** Descriptions:		Enable PWM by setting the PCR, PTCR registers
**
** parameters:			channel number
** Returned value:		None
** 
******************************************************************************/
void PWM_Start( uint32_t channelNum )
{
  if ( channelNum == 1 )
  {
	/* All single edge, all enable */
	LPC_PWM1->PCR = PWMENA1 | PWMENA2 | PWMENA3 | PWMENA4 | PWMENA5 | PWMENA6;
	LPC_PWM1->TCR = TCR_CNT_EN | TCR_PWM_EN;	/* counter enable, PWM enable */
  }
  return;
}

/******************************************************************************
** Function name:		PWM_Stop
**
** Descriptions:		Stop all PWM channels
**
** parameters:			channel number
** Returned value:		None
** 
******************************************************************************/
void PWM_Stop( uint32_t channelNum )
{
  if ( channelNum == 1 )
  {
	LPC_PWM1->PCR = 0;
	LPC_PWM1->TCR = 0x00;		/* Stop all PWMs */
  }
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/

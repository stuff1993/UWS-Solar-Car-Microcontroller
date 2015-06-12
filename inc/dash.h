/*
 * dash.h
 *
 *  Created on: Jun 9, 2015
 *      Author: Stuff
 */

#ifndef DASH_H_
#define DASH_H_

// type.h

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif


typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;
typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

// end type.h

#define WH_RADIUS_M (1)

// TODO: Check pins
/// OUTPUTS
#define BUZZER_ON		LPC_GPIO0->FIOSET = (1<<3);
#define BUZZER_OFF		LPC_GPIO0->FIOCLR = (1<<3);

#define REVERSE_ON		LPC_GPIO1->FIOSET = (1<<26);		// gear4
#define REVERSE_OFF		LPC_GPIO1->FIOCLR = (1<<26);

#define NEUTRAL_ON		LPC_GPIO1->FIOSET = (1<<25);		// gear3
#define NEUTRAL_OFF		LPC_GPIO1->FIOCLR = (1<<25);

#define REGEN_ON		LPC_GPIO1->FIOSET = (1<<24);		// gear2
#define REGEN_OFF		LPC_GPIO1->FIOCLR = (1<<24);

#define DRIVE_ON		LPC_GPIO1->FIOSET = (1<<23);		// gear1
#define DRIVE_OFF		LPC_GPIO1->FIOCLR = (1<<23);

#define BRAKELIGHT_ON	LPC_GPIO1->FIOSET = (1<<21);
#define BRAKELIGHT_OFF	LPC_GPIO1->FIOCLR = (1<<21);

#define DUTYBL			LPC_PWM1->MR6

/// INPUTS
#define INCREMENT 		!(LPC_GPIO0->FIOPIN & (1<<1))
#define DECREMENT		!(LPC_GPIO1->FIOPIN & (1<<27))
#define SELECT 			!(LPC_GPIO1->FIOPIN & (1<<29))
#define RIGHT 			!(LPC_GPIO1->FIOPIN & (1<<28))
#define LEFT 			!(LPC_GPIO0->FIOPIN & (1<<0))

#define FORWARD 		!(LPC_GPIO0->FIOPIN & (1<<10))
#define REVERSE 		!(LPC_GPIO0->FIOPIN & (1<<11))

#define SPORTS_MODE		!(LPC_GPIO2->FIOPIN & (1<<11))


#define IIR_FILTER_GAIN	16

#define PORT_USED	1	// I2C port

#define ECONOMY		(0)
#define SPORTS 		(1)

#define ECONOMY_RAMP_SPEED	5
#define SPORTS_RAMP_SPEED	30
#define REGEN_RAMP_SPEED	30

#define FLAG(x) unsigned int x :1;
#define ON		(1)
#define OFF		(0)

/// EEPROM Addresses ///
#define AddressBUZZ 	0
#define AddressBL 		4
#define AddressODOF		8
#define AddressODOR		12
#define AddressMPPT1WHR	16
#define AddressBMUWHR	20
#define AddressMPPT2WHR	24


void BOD_IRQHandler(void);
void pollMPPTCAN(void);
void extractMPPTDATA(MPPT *_MPPT, fakeMPPTFRAME *_fkMPPT);
void checkBUTTONS(void);
void driveROUTINE(void);
void checkMPPTCOMMS(void);
void tx500CAN(void);
void doCALCULATIONS(void);
void recallVariables(void);
void storeVariables(void);
uint32_t EERead(uint32_t EERadd);
void EEWrite(uint32_t add, uint32_t data);
uint32_t I2CRead(uint32_t eeaddress);
void I2CWrite(uint32_t eeaddress, uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3);
uint32_t iirFILTER(uint32_t newDATA, uint32_t oldDATA);
void setPORTS(void);
void buzzer(uint32_t val);
void BOD_Init(void);

#endif /* DASH_H_ */

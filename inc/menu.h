/*
 * menu.h
 *
 *  Created on: 29 May 2015
 *      Author: Stuff
 */

#ifndef MENU_H_
#define MENU_H_

#define	MENU_ITEMS 15
#define ERROR_ITEMS 2

void displayERRORonSTART(void);
void displayINTRO(void);

void displayINFO(void);
void displayBUSDATA(void);
void displayHOME(void);
void displayDRIVE(void);
void displayCRUISE(void);
void displayMPPT1(void);
void displayMPPT2(void);
void displayMPPT_MAH(void);
void displayMOTOR(void);
void displayDEBUG(void);
void displayERRORS(void);
void displayOTHER(void);
void displayPEAKS(void);
void displayRUNTIME(void);
void displayODOMETER(void);
void displaySWOC(void);
void _lcd_putTitle(char*);

void menuInit (void);

#endif /* MENU_H_ */

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

void lcd_display_errOnStart(void);
void lcd_display_intro(void);

void lcd_display_info(void);
void lcd_display_escBus(void);
void lcd_display_home(void);
void lcd_display_drive(void);
void lcd_display_cruise(void);
void lcd_display_MPPT1(void);
void lcd_display_MPPT2(void);
void lcd_display_MPPTPower(void);
void lcd_display_motor(void);
void lcd_display_debug(void);
void lcd_display_errors(void);
void lcd_display_other(void);
void lcd_display_peaks(void);
void lcd_display_runtime(void);
void lcd_display_odometer(void);

void lcd_display_SWOC(void);
void lcd_display_HWOC(void);

void _lcd_putTitle(char*);

void menuInit (void);

#endif /* MENU_H_ */

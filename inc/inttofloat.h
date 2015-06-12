/*
 * inttofloat.h
 *
 *  Created on: Jun 9, 2015
 *      Author: Stuff
 */

#ifndef INTTOFLOAT_H_
#define INTTOFLOAT_H_

union
{
	uint32_t 	INPUT_INT;
	float		OUTPUT_FLOAT;
}toFloat;

float convertToFloat(uint32_t tempvalF){toFloat.INPUT_INT = tempvalF;return toFloat.OUTPUT_FLOAT;}
float convertToUint(float tempvalF){toFloat.OUTPUT_FLOAT = tempvalF;return toFloat.INPUT_INT;}

#endif /* INTTOFLOAT_H_ */

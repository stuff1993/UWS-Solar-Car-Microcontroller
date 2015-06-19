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
	uint32_t 	_uint;
	int32_t		_int;
	float		_float;
}_converter;

float conv_uint_float(uint32_t _inVal){_converter._uint = _inVal;return _converter._float;}
float conv_int_float(int32_t _inVal){_converter._int = _inVal;return _converter._float;}

uint32_t conv_float_uint(float _inVal){_converter._float = _inVal;return _converter._uint;}
uint32_t conv_int_uint(int32_t _inVal){_converter._int = _inVal;return _converter._uint;}

int32_t conv_float_int(float _inVal){_converter._float = _inVal;return _converter._int;}
int32_t conv_uint_int(uint32_t _inVal){_converter._uint = _inVal;return _converter._int;}

#endif /* INTTOFLOAT_H_ */

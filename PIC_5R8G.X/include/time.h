/*time.h*/

#ifndef __time_h__
#define __time_h__

#include "typedefine.h"

#define _XTAL_FREQ 7370000	//セラロックの周波数

//関数の宣言
void delay_ms(UWORD);
void delay_us(UWORD);

#endif	//#ifndef __time_h__
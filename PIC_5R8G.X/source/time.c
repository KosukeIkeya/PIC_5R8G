/*** ���Ԑ��� ***/

#include <htc.h>
#include "pic16f886.h"

#include "typedefine.h"
#include "time.h"

//ms�E�F�C�g�֐�
void delay_ms(UWORD msec)
{
	while(msec)
	{
		__delay_ms(1);
		msec--;
	}	
}

//us�E�F�C�g�֐�
void delay_us(UWORD usec)
{
	while(usec)
	{
		__delay_us(1);
		usec--;
	}	
}
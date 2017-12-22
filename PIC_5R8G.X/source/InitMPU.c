/*** �}�C�R����IO�|�[�g�ݒ� ***/

#include <xc.h>
//#include "pic16f886.h"
#include "InitMPU.h"

/*** �}�C�R������������ ***/
void init_mpu(void)
{
	//�|�[�g�̏�����
	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00;	
	
	//AD�ݒ�i�S�ăf�W�^�����́j
	ANSEL  = 0x00;	//AD�ݒ�
	ANSELH = 0x00;	//AD�ݒ�
	
	//�|�[�g���o�͐ݒ�	
	TRISA  = 0xC0;	//���o�͐ݒ�
	TRISB  = 0x2B;	//���o�͐ݒ�
    TRISC  = 0x84;	//���o�͐ݒ�
	
	//�|�[�g�����l�ݒ�		
	PORTA  = 0x21;	//�����l�ݒ�
	PORTB  = 0x94;	//�����l�ݒ�
	PORTC  = 0x41;	//�����l�ݒ�
	
}

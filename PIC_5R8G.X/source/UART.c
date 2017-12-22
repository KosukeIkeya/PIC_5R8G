#include <xc.h>
//#include "pic16f886.h"
//#include <stdlib.h>

//#include "typedefine.h"
#include "UART.h"
#include "time.h"
#include "FROM.h"
//#include "Main.h"
#include "MAX2828.h"
#include "InitMPU.h"


//�ϐ��̐錾
static UBYTE send_buf[6];	//���M�p�o�b�t�@
static UBYTE rData[12];		//��M�p�o�b�t�@
static USLONG dlength;		//�f�[�^��

extern UDWORD               g_data_adr;	//FROM��̃A�h���X(�f�[�^�p)

static bank2 volatile CamDataBuf	Rbuf2;	//�摜�f�[�^�p�o�b�t�@
static bank3 volatile CamDataBuf	Rbuf3;	//�摜�f�[�^�p�o�b�t�@

//�萔�̐錾
const UBYTE syncWord[6]   = {0xAA,0x0D,0x00,0x00,0x00,0x00};
const UBYTE ACK0[6]       = {0xAA,0x0E,0x00,0x00,0x00,0x00};
const UBYTE AckEnd[6]     = {0xAA,0x0E,0x00,0x00,0xF0,0xF0};
const UBYTE InitCam[6]    = {0xAA,0x01,0x04,0x07,0x00,0x07};
const UBYTE changeSize[6] = {0xAA,0x06,0x08,0x80,0x00,0x00};	//1�p�P�b�g=128�o�C�g
const UBYTE SnapShot[6]   = {0xAA,0x05,0x00,0x00,0x00,0x00};
const UBYTE GetPicCom[6]  = {0xAA,0x04,0x01,0x00,0x00,0x00};

const UBYTE PRBS9[64] =	   {0x21, 0x86, 0x9c, 0x6a, 0xd8, 0xcb, 0x4e, 0x14,
							0x6a, 0xf9, 0x4d, 0xd2, 0x7e, 0xb2, 0x32, 0x03,
							0xc6, 0x14, 0x4b, 0x7f, 0xd1, 0xb8, 0xa6, 0x79,
							0x7c, 0x17, 0xac, 0xed, 0x06, 0xad, 0xaf, 0x0a,
							0x94, 0x7a, 0xba, 0x03, 0xe7, 0x92, 0xd7, 0x15,
							0x09, 0x73, 0xe8, 0x6d, 0x16, 0xee, 0xe1, 0x3f,
							0x78, 0x1f, 0x9d, 0x09, 0x52, 0x6e, 0xf1, 0x7c,
							0x36, 0x2a, 0x71, 0x6c, 0x75, 0x64, 0x44, 0x80
};

const UBYTE STR[] = {"ABCDEFGH\r\n"};

//�֐��̐錾
//static void		initbau(void);
void            initbau(UBYTE);
UBYTE    getUartData(void);
void            sendChar(UBYTE);
void            send_01(void);
void            send_OK(void);
void            send_NG(void);
static void		syncCam(void);
static void		sendCom(const UBYTE *);
static void		changePackageSize(void);
static void		snap(void);
static USLONG	sendGetPicCom(void);
static void		sendAckID(UBYTE *);
static void		saveCamPack(UDWORD *);
static void		loadCamPack(UDWORD *);
static UBYTE	mk_pn9(void);
static void		set_pn9(void);

//*** SEND�|�[�g�̃`�F�b�N ***
//UBYTE return 0�F�����o�@�A1�F���o
UBYTE CheckSendPort(void)
{
	static UBYTE	cha = (UBYTE)0;
	
	if(SEND == 0)
	{
		if(cha < 30)
		{
			cha++;
			return 0;
		}
		else if(cha >= 30)
		{
			cha = 0;
			return 1;	
		}
	}
	cha = 0;
	return 0;
}

//�J�����̃Z�b�g�A�b�v
void setupCam(UBYTE cam)
{
	//�J�����̑I��
	if(cam)
	{
		CAMERA_SEL = Bit_Low;		//�J����2��I��
	}
	else
	{
		CAMERA_SEL = Bit_High;		//�J����1��I��
	}		
	
	CAMERA_POW = Bit_High;			//�J�����d��ON
	
	initbau(BAU_LOW);				//MPU UART�����ݒ�
	__delay_ms(1000);				//1s�E�F�C�g
	syncCam();						//�J�����Ƃ̓���
	
	BAULATE = BAU_HIGH;				//MPU�̃{�[���[�g��115.2kbps�ɕύX
	
	changePackageSize();			//�p�b�P�[�W�T�C�Y��128BYTE�ɐݒ�
}

//�摜�f�[�^�T�C�Y�̎擾
void getPicSize(void)
{
	snap();							//�B�e
	dlength = sendGetPicCom();		//�f�[�^���̎擾
}

//�摜�f�[�^�T�C�Y�̕ۑ�
UBYTE savePicSize(void)
{
	EXCHG_LONG len;
	EXCHG_LONG tmp;
	UBYTE i = 0;
	UBYTE j = 3;
	UBYTE Ret;
	
	len.us = (UDWORD)dlength;
	for(i=0;i<4;i++)
	{
		tmp.uc[j] = len.uc[i];
		j--;
	}	
	Ret = flash_Write_Page(0UL,(UWORD)4,tmp.uc);
	return Ret;
}

//�摜�f�[�^�ۑ�
void savePicData(void)
{
	UBYTE idCount1 = 0;			//�p�b�P�[�WID(LSB)
	UBYTE idCount2 = 0;			//�p�b�P�[�WID(MSB)
	UWORD i = 0;
//	UBYTE Ret;
	UDWORD w_adr;
	
	w_adr = g_data_adr;
	//ACK0�𑗐M�o�b�t�@�Ƀ��[�h
	for(i=0;i<6;i++)
	{
		send_buf[i] = ACK0[i];
	}
	
	while(dlength > 122)
	{
		//ID�J�E���g���Z
		send_buf[4] = idCount1;
		send_buf[5] = idCount2;
	
		sendAckID(send_buf);		//ACK(�f�[�^�擾ID)���M
	
		//�摜�f�[�^��M
		for(i=0;i<64;i++)
		{
			Rbuf2.Data[i] = getUartData();
		}
		
		for(i=0;i<64;i++)
		{
			Rbuf3.Data[i] = getUartData();
		}
		
		saveCamPack(&w_adr);
	
		dlength -= 122;
		
		//�C���[�W�f�[�^ID�̉��Z����
		if(idCount1 == 0xFF)	//�p�b�P�[�WID��LSB��FF�Ȃ猅�オ��
		{
			idCount1 = 0;
			idCount2++;
		}
		else
		{
			idCount1++;
		}		
	}
	
	/***�ŏI�p�b�P�[�W�̏���***/
	if(dlength != 0)
	{
		//ID�J�E���g���Z
		send_buf[4] = idCount1;
		send_buf[5] = idCount2;
	
		sendAckID(send_buf);		//ACK(�f�[�^�擾ID)���M
		
		if(dlength > 58)
		{
			for(i=0;i<64;i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			dlength = dlength - 60;
			
			for(i=0;i<(dlength+2);i++)
			{
				Rbuf3.Data[i] = getUartData();
			}
			
			for(i=(dlength+2);i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}
			saveCamPack(&w_adr);
		}
		else if(dlength == 58)
		{
			for(i=0;i<64;i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			
			for(i=0;i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}
			
			saveCamPack(&w_adr);
		}
		else
		{
			for(i=0;i<(dlength+6);i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			for(i=(dlength+6);i<64;i++)
			{
				Rbuf2.Data[i] = 0xFF;
			}
			
			for(i=0;i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}
			
			saveCamPack(&w_adr);
		}
		
		sendCom(AckEnd);		
	}
	else
	{
		sendCom(AckEnd);
	}		
}

/* �ϒ��f�[�^���M */
void sendModData(UDWORD RAddr)
{
	UBYTE 		Ret;
	UBYTE		i;
	UBYTE		j;
	EXCHG_LONG	len;
	EXCHG_LONG	tmp;
	
	initbau(BAU_HIGH);								//UART�̏�����
//	BAULATE = BAU_HIGH;						//115.2kbps�ɐݒ�
	CREN = Bit_Low;							//��M�֎~
	
	//�f�[�^���擾
	Ret = flash_Read_Data(0UL,4,len.uc);	//�f�[�^����FROM����ǂݏo��
	j = 3;
	for(i=0;i<4;i++)						//�G���f�B�A���̕ϊ�
	{
		tmp.uc[i] = len.uc[j];
		j--;
	}
	dlength = (USLONG)tmp.us;
	
	Mod_SW = Bit_Low;						//�ϒ�ON
	max2828_txon();
	PA_SW  = Bit_High;
	
	delay_ms(1);
	
	//�ϒ��f�[�^�擾�E���M
	while(dlength > 122)
	{
		loadCamPack(&RAddr);				//�摜�f�[�^��FROM����o�b�t�@�ɓǂݏo��
		
		//�o�b�t�@���̃f�[�^��UART���M
		for(i=0;i<64;i++)
		{
			sendChar(Rbuf2.Data[i]);
		}
		for(i=0;i<64;i++)
		{
			sendChar(Rbuf3.Data[i]);
		}
		dlength -= 122;
	}
	//�ŏI�p�P�b�g�̑��M
	loadCamPack(&RAddr);
	for(i=0;i<64;i++)
	{
		sendChar(Rbuf2.Data[i]);
	}
	for(i=0;i<64;i++)
	{
		sendChar(Rbuf3.Data[i]);
	}
	
	PA_SW = Bit_Low;
	MAX2828_TXEN = Bit_Low;
	Mod_SW = Bit_High;						//�ϒ�OFF
}	

//UART�̏�����
//static void initbau(void)
//{
//	/*�{�[���[�g�ݒ�*/
//	BRGH    = Bit_High;		//�{�[���[�g�������[�h
//	BAUDCTL = 0x08;			//16�{��
//	BAULATE = BAU_LOW;		//14.4kbps
//	SPEN    = Bit_High;		//�V���A���|�[�g�ݒ�
//	TXEN    = Bit_High;		//���M����
//	CREN    = Bit_High;		//��M����
//}

//UART�̏�����
void initbau(UBYTE bau)
{
	/*�{�[���[�g�ݒ�*/
	BRGH    = Bit_High;		//�{�[���[�g�������[�h
	BAUDCTL = 0x08;			//16�{��
	BAULATE = bau;
	SPEN    = Bit_High;		//�V���A���|�[�g�ݒ�
}

//�p�b�P�[�W�T�C�Y�ύX�֐�
static void changePackageSize(void)
{
	UBYTE i = 0;

	sendCom(changeSize);	//�p�b�P�[�W�T�C�Y��128BYTE�ɕύX
	
	//ACK�̋�ǂ�
	for(i=0;i<6;i++)
	{
		rData[i] = getUartData();
	}	
}

//�B�e�R�}���h���M
static void snap(void)
{
	UBYTE i = 0;

	sendCom(SnapShot);	//�p�b�P�[�W�T�C�Y��128BYTE�ɕύX
	
	//Ack�̋�ǂ�
	for(i=0;i<6;i++)
	{
		rData[i] = getUartData();
	}	
}

//�f�[�^���擾
static USLONG sendGetPicCom(void)
{
	UBYTE i = 0;

	sendCom(GetPicCom);	//Jpeg���k�R�}���h���M
	
	for(i=0;i<12;i++)
	{
		rData[i] = getUartData();
	}
	
	return ((USLONG)rData[11]<<16)+((USLONG)rData[10]<<8)+(USLONG)rData[9];	//�f�[�^���̌v�Z
}

//�J���������֐�
static void syncCam(void)
{
	UBYTE i = 0;
	UBYTE j = 0;
	
	for(i = 0;i < 60;i++)
	{
		if(RCIF)	//��M�o�b�t�@�Ƀf�[�^������Ƃ�
		{
			for(j = 0;j < 12;j ++)
			{
				rData[j] = getUartData();
			}
			j = 0;
			
			sendCom(ACK0);		//ACK0���M
			delay_ms(1);
			
			sendCom(InitCam);	//�J�����ݒ�(115.2kbps,VGA)
			delay_ms(1);
			
			//ACK�̋�ǂ�
			for(j = 0;j < 6;j++)
			{
				rData[j] = getUartData();
			}	
			
			delay_ms(50);		//50ms�E�F�C�g
			
			return;
		}
		else
		{
			sendCom(syncWord);	//�������[�h���M
			delay_ms(1);
		}		
	}	
}

//1BYTE��M�֐�
static UBYTE getUartData(void)
{
	UBYTE rbuf;
	while(RCIF != 1);	//��M�����ݑ҂�
	if(FERR == 1)		//�t���[�~���O�G���[����
	{
		rbuf = RCREG;
		return '?';
	}
	else if(OERR == 1)	//�I�[�o�[�����G���[����
	{
		CREN = 0;
		CREN = 1;
		return '?';
	}
	return RCREG;
}

//1Byte���M�֐�
void sendChar(UBYTE c)
{
	while(TXIF != 1);
	TXREG = c;
}

//�J�����R�}���h���M�֐�
static void sendCom(const UBYTE *com)
{
	UBYTE i = 0;

	for(i = 0;i < 6;i++)
	{
		sendChar(com[i]);
	}
	while(!TRMT);	//111125 ���M�I���܂�wait
}

//�f�[�^�擾�pACK���M
static void sendAckID(UBYTE *id)
{
	UBYTE i = 0;
	
	for(i = 0;i < 6;i++)
	{
		sendChar(id[i]);
	}
	while(!TRMT);
}

/* 1�p�P�b�g���̃f�[�^save */
static void saveCamPack(UDWORD * WAddr)
{
	UBYTE Ret;
	
	Ret = flash_Write_Page((UDWORD)*WAddr,(UWORD)64,(UBYTE *)Rbuf2.Data);
	*WAddr += (UDWORD)64;
	Ret = flash_Write_Page((UDWORD)*WAddr,(UWORD)64,(UBYTE *)Rbuf3.Data);
	*WAddr += (UDWORD)64;
}

/* 1�p�P�b�g���̃f�[�^load */
static void loadCamPack(UDWORD * RAddr)
{
	UBYTE Ret;
	
	Ret = flash_Read_Data((UDWORD)*RAddr,64UL,(UBYTE *)Rbuf2.Data);
	*RAddr += 64UL;
	Ret = flash_Read_Data((UDWORD)*RAddr,64UL,(UBYTE *)Rbuf3.Data);
	*RAddr += 64UL;
}
	
/*-----------------------------------------------------------------------*/

//�摜�f�[�^�擾(���ڕϒ��p)
void getPicData(void)
{
	UBYTE idCount1 = 0;			//�p�b�P�[�WID(LSB)
	UBYTE idCount2 = 0;			//�p�b�P�[�WID(MSB)
	UWORD i = 0;
	
	//ACK0�𑗐M�o�b�t�@�Ƀ��[�h
	for(i=0;i<6;i++)
	{
		send_buf[i] = ACK0[i];
	}
	i = 0;
	
	while(dlength > 122)
	{
		//ID�J�E���g���Z
		send_buf[4] = idCount1;
		send_buf[5] = idCount2;
	
		sendAckID(send_buf);		//ACK(�f�[�^�擾ID)���M
	
		//�摜�f�[�^��M
		for(i=0;i<64;i++)
		{
			Rbuf2.Data[i] = getUartData();
		}
		i = 0;
		for(i=0;i<64;i++)
		{
			Rbuf3.Data[i] = getUartData();
		}
		i = 0;
		
		dlength -= 122;
		
		//�C���[�W�f�[�^ID�̉��Z����
		if(idCount1 == 0xFF)	//�p�b�P�[�WID��LSB��FF�Ȃ猅�オ��
		{
			idCount1 = 0;
			idCount2++;
		}
		else
		{
			idCount1++;
		}		
	}
	
	/***�ŏI�p�b�P�[�W�̏���***/
	if(dlength != 0)
	{
		//ID�J�E���g���Z
		send_buf[4] = idCount1;
		send_buf[5] = idCount2;
	
		sendAckID(send_buf);		//ACK(�f�[�^�擾ID)���M
		
		if(dlength > 58)
		{
			for(i=0;i<64;i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			i = 0;
			dlength = dlength - 60;
			for(i=0;i<(dlength+2);i++)
			{
				Rbuf3.Data[i] = getUartData();
			}
			for(i=(dlength+2);i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}	
		}
		else if(dlength == 58)
		{
			for(i=0;i<64;i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			i = 0;
			for(i=0;i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}	
		}
		else
		{
			for(i=0;i<(dlength+6);i++)
			{
				Rbuf2.Data[i] = getUartData();
			}
			for(i=(dlength+6);i<64;i++)
			{
				Rbuf2.Data[i] = 0xFF;
			}
			i = 0;
			for(i=0;i<64;i++)
			{
				Rbuf3.Data[i] = 0xFF;
			}
		}
		
		sendCom(AckEnd);		
	}
	else
	{
		sendCom(AckEnd);
	}		
}

//�����񑗐M�֐�
/*
void sendString(const UBYTE* p)
{
	while(*p)
		sendChar(*p++);
}
*/

static UBYTE mk_pn9(void)
{
	static UWORD	reg = 0x0021;
	UWORD			out,fb;

	out = reg & 0x0001;
	fb = (reg ^ (reg >> 4U)) & 0x0001;
	reg = (reg >> 1) | (fb << 8);
	
	return((UBYTE)out);
}

static void set_pn9(void){
	UBYTE i;
	
    send_buf[0] = 0x00;
	for(i = 0; i < 8; i++){
		send_buf[0] |= mk_pn9() << i;
	}

//	return(send_buf[0]);
}

/* PN9�f�[�^���M */
void send_pn9(void)
{
//	UWORD i;
	UBYTE i;
	
	initbau(BAU_HIGH);								//UART�̏����� 115.2kbps
//	initbau(BAU_LOW);								//14.4kbps
//	initbau(0x5F);									//19.2kbps
//	initbau(0x1F);									//57.6kbps	
	CREN = Bit_Low;									//��M�֎~
	
//	Mod_SW = Bit_Low;						//�ϒ�ON
//	max2828_txon();
//	PA_SW  = Bit_High;
	
//	delay_ms(1);
	
	while(1){
//	for(i = 0; i < 10000; i++){
//        set_pn9();
//		sendChar(send_buf[0]);
		for(i=0; i<64; i++){
			send_buf[0] = PRBS9[i];
			sendChar(send_buf[0]);
		}
		i = 0;
	}
}

void send_55(void){
	initbau(BAU_HIGH);								//UART�̏����� 115.2kbps
	CREN = Bit_Low;									//��M�֎~
	send_buf[0] = 0x55;
//	send_buf[0] = 0x00;
	while(1){
		sendChar(send_buf[0]);
	}
}

void send_tst_str(void){
    CREN = Bit_Low;
    TXEN = Bit_High;
	UBYTE testcount_1 = 0,testcount_2 = 0;
//	initbau(BAU_HIGH);								//UART�̏����� 115.2kbps
//	initbau(0x1F);									//57.6kbps
//	initbau(0x5F);									//19.2kbps
	while(1){
        if(testcount_2 = 0){
            send_01();
            /*
            for(UINT i=0;i<10;i++){
                sendChar(0xFF);
                __delay_us(20);
                sendChar(0xD8);
                __delay_us(20);
            }*/
        }else if(testcount_2 > 50 ){
			CLRWDT();
			WDT_CLK = ~WDT_CLK;
			testcount_2 = 0;
            delay_ms(1000);
		}else if(testcount_2 <= 50 ){
			for(UINT i=0;i<10;i++){
                //send_01();
                send_buf[0] = STR[i];
                sendChar(send_buf[0]);
                //sendChar(0x00);
                __delay_us(20);
            }
            /*
            sendChar(0xFF);
            delay_us(20);
            sendChar(0xD8);
            delay_us(20);*/
		}
        testcount_2 ++;
	}
}

void send_01(void){
    UBYTE Preamblecount = 0;
    for(Preamblecount=0 ;Preamblecount <100; Preamblecount++){
        sendChar('0');
        __delay_us(20);
        sendChar('1');
        __delay_us(20);
    }
    sendChar('\r');
    sendChar('\n');
}

void send_OK(void){
    sendChar('O');
    __delay_us(20);
    sendChar('K');
    __delay_us(20);
}

void send_NG(void){
    sendChar('N');
    __delay_us(20);
    sendChar('G');
    __delay_us(20);
}

void echo_back(void){
    UBYTE testbuf1;
    UBYTE testbuf2;
    UDWORD echo_adr = g_data_adr;
    CREN = Bit_High;
    TXEN = Bit_High;
    flash_Erase(g_data_adr,S_ERASE);    //g_data_adr��sector65536byte���폜
    while(1){
        while(RCIF != 1);
        testbuf1 = RCREG;
        flash_Write_Data(echo_adr,1UL,&testbuf1);
        flash_Read_Data(echo_adr,1UL,&testbuf2);
        sendChar(testbuf2);
        echo_adr += 1UL;
    }
}
#include <xc.h>
//#include "pic16f886.h"

#include "InitMPU.h"
#include "MAX2828.h"
#include "UART.h"
#include "time.h"
#include "FROM.h"

/*�R���t�B�M�����[�V�����r�b�g�̐ݒ�*/
// CONFIG1
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
//#pragma config WDTE = ON       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR21V   // Brown-out Reset Selection bit (Brown-out Reset set to 2.1V)
#pragma config WRT = HALF       // Flash Program Memory Self Write Enable bits (0000h to 0FFFh write protected, 1000h to 1FFFh may be modified by EECON control)

UDWORD          g_data_adr  = (UDWORD)0x00000000;


#define JPGCOUNT 5000

void main(void){
    UDWORD			g1_data_adr = (UDWORD)0x00010000;	//FROM��̃A�h���X(�f�[�^�p)
    UDWORD			g2_data_adr = (UDWORD)0x00020000;	//FROM��̃A�h���X(�f�[�^�p)
    
    UBYTE Rxdata[20];
    UBYTE Txdata[20];
    UDWORD FROM_Write_adr = g1_data_adr; //EEPROM�������ݗp�A�h���X
    UDWORD FROM_Read_adr  = g1_data_adr; //EEPROM�ǂݍ��ݗp�A�h���X
    UDWORD Roop_adr       = g1_data_adr; //EEPROM���[�v�p�A�h���X
    UINT roopcount = 0;
    
    init_mpu();                         //�|�[�g�̏�����
    //initbau(BAU_HIGH);                  //115200bps
    //initbau(0x1f);                      //57600bps
    initbau(BAU_LOW);                   //14400bps
    MAX2828_EN = 1;                     //�S�d��ON
    __delay_us(100);                      //100us wait
    FLASH_SPI_EI();                     //SPI�ʐM����    
    init_max2828();                     //MAX2828�̏����ݒ�
    Mod_SW = 0;                         //���M�ϒ��iFSK�ϒ��jON
    
    //flash_Erase(g_data_adr,B_ERASE);
    
//    CAMERA_POW = 0;
//    CAMERA_SEL = 0;
//    max2828_txon();
//    send_pn9();            //PN9���M
//    send_55();             //�v���A���u�����M
    send_tst_str();        //�e�X�g�������M
    
    
    while(1){
        flash_Erase(g1_data_adr,S_ERASE);    //g1_data_adr��sector65536byte���폜
        flash_Erase(g2_data_adr,S_ERASE);    //g2_data_adr��sector65536byte���폜
        
        // Camera�|�[�g�ɐݒ�
        CAMERA_POW = 1;
        CAMERA_SEL = 1;
        CREN = Bit_High;
        TXEN = Bit_Low;
        MAX2828_TXEN = 0;
        PA_SW = 0;
        
        //send_tst_str();   //  �e�X�g�����񑗐M
        //echo_back();      //�@�G�R�[�o�b�N�iUART&FROM�i�[�j
        
        //  ���[�v�A�h���X�����̋󂫃o�b�t�@�ɃZ�b�g
        Roop_adr = g1_data_adr;
        //  �������݃A�h���X�����[�v�A�h���X�ɃZ�b�g
        FROM_Write_adr = Roop_adr;
        //send_OK();
        //  UART��M��EEPROM�Ɋi�[
        for(UINT j=0;j<JPGCOUNT;j++){
            for(UINT i=0;i<20;i++){
                while(RCIF != 1);
                Rxdata[i] = RCREG;
            }
            flash_Write_Data(FROM_Write_adr,20UL,&Rxdata);
            FROM_Write_adr += 20UL;
        }
        //send_NG();
        
        // Amp�|�[�g�ɐݒ�
        /**/
        CAMERA_POW = 0;
        CAMERA_SEL = 0;
        max2828_txon();
        CREN = Bit_Low;
        TXEN = Bit_High;
        delay_ms(10);
        
        //  �ǂݍ��݃A�h���X�����[�v�A�h���X�ɃZ�b�g
        FROM_Read_adr = Roop_adr;
        //  �v���A���u�����M
        send_01();
        //  EEPROM������o����UART���M
        for(UINT j=0;j<JPGCOUNT;j++){
            UINT sendcount = 0;
            flash_Read_Data(FROM_Read_adr,20UL,&Txdata);
            
            for(UINT i=0;i<20;i++){
                sendChar(Txdata[i]);
                //sendChar(0x00);
                __delay_us(20);
            }
            FROM_Read_adr += 20UL;
            /*
            sendcount ++;
            if(sendcount == 60){
                __delay_ms(50);
                sendcount = 0;
            }*/
        }
        send_01();
        sendChar('\r');
        sendChar('\n');
        g1_data_adr += (UDWORD)0x00020000;
        g2_data_adr += (UDWORD)0x00020000;
        
        //  �O�t��WDT�L���֐�
        /*
        roopcount++;
        if(roopcount == 29)
        {
            //CLRWDT();
            //WDT_CLK = ~WDT_CLK;
            //  �������C�J�E���^���Z�b�g
            roopcount = 0;
            g1_data_adr = (UDWORD)0x00010000;
            //g2_data_adr = (UDWORD)0x00020000;
        }*/
    }
}
/*-------------------------------------------ͷ�ļ�-------------------------------------------*/
#include <STC8.H>
#include "intrins.h"		//��Ƭ��C����ʹ�û��ָ���ͷ�ļ���
/*-------------------------------------------�궨��-------------------------------------------*/
typedef unsigned char u8;	//�޷��ŵ� 8λ������
typedef unsigned int  u16;	//�޷��ŵ�16λ������
typedef unsigned long u32;	//�޷��ŵ�32λ������
/*-----------------------------------------BOOT�Ŷ���-----------------------------------------*/
#define	BOOT_PIN		P00	//BOOT������ΪP0.0�š�
#define	BOOT_RUN_APP	0	//BOOT��Ϊ�͵�ƽ��ʱ������APP��
/*------------------------------------------���ܶ���------------------------------------------*/
sfr			IAP_TPS			=0xf5;	//EEPROM�����ȴ�ʱ����ƼĴ���
u8	code	INIT_MASK[6]	={0xff,0xff,0xff,0xff,0xff,0xff};//��ʼ�����룬����FLASH���Ծ���0xFF��
void	(* jmp_app)(void);			//��ת������
#define	BOOT_STATUS			0xFE00	//FE00		��BootLoader��״̬
#define	BOOT_JMP_ADDR		0xFE01	//FE01~FE02	��BootLoader����ת��ַ

#define	APP_START			0xFE03	//FE03		��APP����ʼ��ַ
#define	APP_MAX				5		//APP��ռ�ÿռ�Ϊ6�ֽ�
#define	APP_STATUS			0xFE03	//FE03		��APP��״̬
#define	APP_SIZE			0xFE04	//FE04~FE05	��APP����С
#define	APP_JMP_ADDR		0xFE06	//FE06~FE07	��APP��ַ

#define	DATA_MAX			124				//APP�������ֵ����λΪ��������һ����Ϊ512�ֽڡ�
#define	FLASH_SIZE			(DATA_MAX*512)	//APP�������ֵ����λΪ�ֽڡ�

#define	read_jmp__addr		(*(unsigned int  code *)(0x0001))		//��ȡ�����������ת��ַ��
#define	read_jmp__addr_h	(*(unsigned char code *)(0x0001))
#define	read_jmp__addr_l	(*(unsigned char code *)(0x0002))

#define	read_boot_addr		(*(unsigned int  code *)(BOOT_JMP_ADDR))//��ȡBootloader����ת��ַ��
#define	read_boot_addr_h	(*(unsigned char code *)(BOOT_JMP_ADDR))
#define	read_boot_addr_l	(*(unsigned char code *)(BOOT_JMP_ADDR+1))

#define	read_app__addr		(*(unsigned int  code *)(APP_JMP_ADDR))	//��ȡAPP����ת��ַ��
#define	read_app__addr_h	(*(unsigned char code *)(APP_JMP_ADDR))	
#define	read_app__addr_l	(*(unsigned char code *)(APP_JMP_ADDR+1))

#define	read_app__size		(*(unsigned int  code *)(APP_SIZE))		//��ȡAPP�Ĵ�С��
#define	read_app__size_h	(*(unsigned char code *)(APP_SIZE))
#define	read_app__size_l	(*(unsigned char code *)(APP_SIZE+1))

#define	read_app__flag		(*(unsigned char code *)(APP_STATUS))	//��ȡAPP��״̬��
#define	read_boot_flag		(*(unsigned char code *)(BOOT_STATUS))	//��ȡBootLoader��״̬��
//λ����	[		D7		][			D6			][	D5	][	D4	][	D3	][	D2	][	D1	][	D0	]
//APP	[	ʹ�ܱ�־λ	][	��װ  ��ɱ�־λ		][						Ԥ��					]
//BOOT	[	ʹ�ܱ�־λ	][	��ʼ����ɱ�־λ		][						Ԥ��					]
#define	BIT_ENABLE		0x80					//ʹ��λ��
#define	SET_ENABLE		(~(u8)(BIT_ENABLE))		//ʹ�ܸ�����
#define	GET_ENABLE		( (u8)(BIT_ENABLE))		//��ѯʹ��λ��

#define	BIT_DONE		0x40					//���λ��
#define	SET_DONE		(~(u8)(BIT_DONE))		//������������趨���ܡ�
#define	GET_DONE		( (u8)(BIT_DONE))		//��ѯ���λ��

#define	PD_APP__OK		(GET_ENABLE|GET_DONE)	//�ж�APP���Ƿ��кϷ��ĳ���
#define	APP__OK			(0)						//APP���ǺϷ��ĳ���
/*------------------------------------------��������------------------------------------------*/
#if 1//���ڲ���
/*-------------------------------------------------------
��������hard_init
��  ����Ӳ�������ʼ��������
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
2020-12-08���ϲ��˴��ںͶ�ʱ���ĳ�ʼ��������Ϊhard_init��
-------------------------------------------------------*/
void hard_init(void){	
	SCON = 0x50;		//8λ����,�ɱ䲨����
	AUXR = 0x40;		//��ʱ��1ʱ��ΪFosc,��1T
	TMOD = 0x00;		//�趨��ʱ��1Ϊ16λ�Զ���װ��ʽ
	TL0 = 0xC0;			//���ö�ʱ��ֵ
	TH0 = 0x63;			//20ms
	TL1 = 0xCC;			//�趨��ֵ
	TH1 = 0xFF;			//�趨��ֵ
	TF0 = 0;			//���TF0��־
	TF1 = 0;			//���TF1��־
	TR0 = 0;
	TR1 = 1;			//������ʱ��1
	ET1 = 0;			//��ֹ��ʱ��1�ж�
}
/*-------------------------------------------------------
��������send_data
��  �������ڷ��͵��ֽں�����
��  �룺	dat	-	Ҫ���͵��ֽڡ�
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
-------------------------------------------------------*/
void send_data(u8 dat){
    SBUF=dat;	//����һ�����ݡ�
	while(!TI);	//�ȴ����ݷ��ꡣ
	TI=0;		//������ͱ�־λ��
}
/*-------------------------------------------------------
��������send_string
��  �������ڷ����ַ���������
��  �룺	str	-	Ҫ���͵��ַ�����
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
2020-12-08���Ż��˺�����ռ�ÿռ䡣
-------------------------------------------------------*/
void send_string(u8 code * str){
	while(*str){	//�ж��ַ����Ƿ�ͷ��
		SBUF=*str++;//����һ�����ݡ�
		while(!TI);	//�ȴ����ݷ��ꡣ
		TI=0;		//������ͱ�־λ��
	}	
}
/*-------------------------------------------------------
��������send_num
��  �������ڷ���16λ���ݺ��������͸�ʽΪ%u��
��  �룺	dat	-	Ҫ���͵����ݡ�
�����ߣ�����
�������̣���
�������ڣ�2020-12-03
�޸ļ�¼��
-------------------------------------------------------*/
void send_num(u16 dat){
	u8 n[5],i;
	for(i=0;i<5;i++){
		n[i]=dat%10;
		dat/=10;
	}
	if(n[4]){
		send_data('0'+n[4]);
	}
	if(n[4]|n[3]){
		send_data('0'+n[3]);
	}
	if(n[4]|n[3]|n[2]){
		send_data('0'+n[2]);
	}
	if(n[4]|n[3]|n[2]|n[1]){
		send_data('0'+n[1]);
	}
	send_data('0'+n[0]);
}
#endif
#if 1//FLASH��������
/*-------------------------------------------------------
��������eeprom_off
��  ����eeprom�رպ�����
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
-------------------------------------------------------*/
void eeprom_off(void){
    IAP_CONTR = 0;   //�ر�IAP����
    IAP_CMD   = 0;   //�������Ĵ���
    IAP_TRIG  = 0;   //��������Ĵ���
    IAP_ADDRH = 0xff;//����ַ���õ���IAP����
    IAP_ADDRL = 0xff;//����ַ���õ���IAP����
}
/*-------------------------------------------------------
��������eeprom_erase
��  ����eeprom�������������ڲ���һ��������
��  �룺	addr	-	Ҫ�����ĵ�ַ��
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
-------------------------------------------------------*/
void eeprom_erase(u16 addr){
	IAP_CONTR = 0x81;		//ʹ��IAP
	IAP_CMD   = 3;			//����IAP��������
	IAP_ADDRL = addr;		//����IAP�͵�ַ
	IAP_ADDRH = addr >> 8;	//����IAP�ߵ�ַ
	IAP_TRIG  = 0x5a;		//д��������(0x5a)
	IAP_TRIG  = 0xa5;		//д��������(0xa5)
	_nop_();				//���Եȴ�һ��
	eeprom_off();			//�ر�IAP����
}
/*-------------------------------------------------------
��������eeprom_write
��  ����eeprom���ֽ�д������������ָ����ַдһ�����ݡ�
��  �룺	addr	-	Ҫд��ĵ�ַ��
		dat		-	Ҫд������ݡ�
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
-------------------------------------------------------*/
void eeprom_write(u16 addr,u8 dat){
	IAP_CONTR = 0x81;		//ʹ��IAP
	IAP_CMD   = 2;			//����IAPд����
	IAP_ADDRL = addr;		//����IAP�͵�ַ
	IAP_ADDRH = addr >> 8;	//����IAP�ߵ�ַ
	IAP_DATA  = dat;		//дIAP����
	IAP_TRIG  = 0x5a;		//д��������(0x5a)
	IAP_TRIG  = 0xa5;		//д��������(0xa5)
	_nop_();				//���Եȴ�һ��
	eeprom_off();			//�ر�IAP����
}
/*-------------------------------------------------------
��������eeprom_write_boot_area
��  ����eeprom���ֽ�дϵͳ��������������ָ������д������ݡ�
��  �룺	addr	-	Ҫд��ĵ�ַ������Ҫϵͳ�����ڣ�
		buf		-	Ҫд������ݻ��棻
		len		-	Ҫд�������������
�����ߣ�����
�������̣���
�������ڣ�2020-11-30
�޸ļ�¼��
2020-12-03���Ż��˹��ܣ�����ֻΪ�̶���������Ѽ�Сռ�ÿռ䣬
			����Ϊeeprom_write_boot_area��
-------------------------------------------------------*/
volatile u8 xdata eeprom_buf[512];//�����������ݵĻ���
void eeprom_write_boot_area(u16 addr,u8 * buf,u16 len){
	u16 i,ad;
	for(i=0;i<512;i++){				//��ϵͳ��ԭ�������ݶ��������ŵ����浱�С�
		eeprom_buf[i]=(*(unsigned char code *)(BOOT_STATUS+i));
	}
	eeprom_erase(BOOT_STATUS);			//���ϵͳ����
	ad=addr-BOOT_STATUS;					//��ȡ��ַ��ƫ������
	for(i=0;i<len;i++){				//������д�뵽���档
		eeprom_buf[i+ad]=buf[i];
	}
	for(i=0;i<512;i++){				//������д�ص���Ƭ����
		eeprom_write(BOOT_STATUS+i,eeprom_buf[i]);
	}
}
#endif

volatile u16 data_count=0,proj_count=0;//�������ݼ������������ݼ���������
/*-------------------------------------------------------
��������data_save
��  �������ݱ��溯�������ڱ���Ӵ��ڽ��յ������ݡ�
�����ߣ�����
�������̣���
�������ڣ�2020-12-24
�޸ļ�¼��
-------------------------------------------------------*/
void data_save(void){
	u16 i;
	if(proj_count<=256){											//����ܼ���С��256��˵���ǵ�һ�����ݡ�
		eeprom_write(APP_JMP_ADDR  ,eeprom_buf[1]);					//��һ������ͨ������������ת��ַ��
		eeprom_write(APP_JMP_ADDR+1,eeprom_buf[2]);					//����ת��ַ����������
		for(i=3;i<proj_count;i++){									//Ȼ��д��ʣ�µ����ݡ�
			eeprom_write(i,eeprom_buf[i]);
		}
	}else{															//������ǵ�һ�����ݣ�
		for(i=0;i<data_count;i++){									//�Ǿ�ֱ�ӱ��浽flash�����ˡ�
			eeprom_write(i+(proj_count-data_count),eeprom_buf[i]);
		}
	}
}

void main(void){
	u16 i;		//16λ��ʱ������
	u8 timeout;	//ʱ�䳬ʱ��������
	IAP_TPS=24;	//STC8G��STC8H�����ã�Ĭ��24MHz��
/*------------------------------------------BootLoader��ʼ��-------------------------------*/
	if(read_boot_flag&GET_ENABLE){											//�ж��ǲ��ǵ�һ������bootloader��					
		eeprom_write(BOOT_STATUS,		SET_ENABLE);						//�ǵĻ�����λʹ�ܱ�־��
		eeprom_write( APP_JMP_ADDR,		(*(unsigned char code *)(0x0001)));	//��ʼ��APP����ת��ַ��
		eeprom_write( APP_JMP_ADDR+1,	(*(unsigned char code *)(0x0002)));	//
		eeprom_write(BOOT_JMP_ADDR,		(*(unsigned char code *)(0x0001)));	//��ʼ��BOOT����ת��ַ��
		eeprom_write(BOOT_JMP_ADDR+1,	(*(unsigned char code *)(0x0002)));	//
		eeprom_write(BOOT_STATUS,		SET_DONE);							//��λ��ʼ����ϱ�־��
	}																		//����BootLoader��ʼ����ɡ�
/*------------------------------------------APP���---------------------------------------*/	
	jmp_app=(u32)read_boot_addr;				//Ĭ�ϰ�BootLoader�ĵ�ַ��APP��ַ����ֹ����ת��
	if((read_app__flag&PD_APP__OK)==APP__OK){	//�жϳ����Ƿ�Ϸ���
		jmp_app=(u32)read_app__addr;			//����Ϸ�����APP��ת��ַ����ת������
		if(BOOT_PIN==BOOT_RUN_APP){jmp_app();}	//BOOT��������ת��APP�ĵ�ƽ����ת��APP��
	}
/*-----------------------------------BootLoader����--------------------------------------*/
	hard_init(); 					//��ʼ�����ںͶ�ʱ����
	send_string("[Boot](0)\r\n");	//��ʾ������BOOT״̬��
	while(!RI);						//�ȴ����ս����롣
	RI=0;							//������ձ�־λ��
	if(SBUF!='!'){					//������ǽ����룬˵�����ڴ�ʱ���Ҵ����ݵġ�
		send_string("[Lock](1)\r\n");//��ʾҪ������Ƭ����
		while(1);					//��ѭ����������Ƭ�����ܼ������ء�
	}
/*--------------------------------------���ؽ���------------------------------------------*/
	send_string("[Erase](2)\r\n");									//��ʾ��ʼ������
	eeprom_erase(0x0000);											//�Ȳ���һ������
	eeprom_write(0x0000,0x02);										//����д����תָ�
	eeprom_write(0x0001,(*(unsigned char code *)(BOOT_JMP_ADDR)));	//ȡ��BOOT����ת��ַ��8λ��д�뵽ָ����ַ�С�
	eeprom_write(0x0002,(*(unsigned char code *)(BOOT_JMP_ADDR+1)));//ȡ��ַ��8λ���ֳ��������ֽ�д���������Լӿ������ٶȡ�
	eeprom_write_boot_area(APP_START,INIT_MASK,APP_MAX);			//��ʼ��APP״̬����
	eeprom_write(APP_STATUS,SET_ENABLE);							//д��APPʹ�ܱ�־λ��
	for(i=512;i<FLASH_SIZE;i+=512){									//���������������
		eeprom_erase(i);
	}
	data_count=0;				//���㵥�����ݼ�������
	proj_count=0;				//�����������ݼ�������
	send_string("[Start](3)");	//��ʾ��ʼ���ء�
	TR0=1;						//�򿪶�ʱ���������ó�ʱ�жϡ�
	timeout=250;				//��һ�γ�ʱʱ��Ϊ250*20ms=5000ms=5s����λ�����������Ҫʱ�䣬��һ�γ�ʱ�ж�Ҫ��һЩ��
	while(1){															//��ѭ����������ݡ�
		while(!RI){														//�ȴ����������ݡ�
			if(TF0){													//ÿ����ʱ�����ʱ��
				TF0=0;													//�����־λ��
				timeout--;												//��ʱʱ��-1��
				if(timeout==0){											//��ʱ�����0��ʱ��˵����ʱ�ˡ�
					TR0=0;												//�رն�ʱ����
					if(proj_count){										//����ڼ���������ݣ�
						send_string("\r\n[Get ");						//��ʾ�����˶������ݡ�
						send_num(proj_count);
						send_string(" Byte](4)\r\n");					//��λ���ֽڡ�
						data_save();									//������Щ���ݣ������ﱣ���ͨ�������һ�������ݡ�
						eeprom_write(APP_SIZE,	(u8)(proj_count>>8));	//��ȡAPP�Ĵ�С��8λ��д�뵽ָ����ַ�С�
						eeprom_write(APP_SIZE+1,(u8)(proj_count));		//��ȡAPP�Ĵ�С��8λ���ֳ��������ֽ�д���������Լӿ������ٶȡ�
						eeprom_write(APP_STATUS,SET_DONE);				//��װ��ɣ�д��ɱ�־λ��
						jmp_app=(u32)read_app__addr;					//��APP��ת��ַ����ת������
					}else{												//����ڼ�û�н��յ����ݣ�
						send_string("\r\n[Time Out](5)\r\n");			//��ʾ��ʱ��
						jmp_app=(u32)read_boot_addr;					//��BOOT����ת��ַ����ת������
					}
					jmp_app();											//���ؽ�����ϣ���ת��
				}
			}
		}
		TF0=0;										//������յ��������ݣ�ҲҪ���㶨ʱ����־λ��
		RI=0;										//ͬʱ������ձ�־λ��
		eeprom_buf[data_count++]=SBUF;				//�����յ����ݱ���������
		proj_count++;								//ͳ��Ŀǰ�ѽ��յ����ݴ�С��
		if(data_count==256){						//sscom5��λ����256�ֽڷ�һ��������ÿ�յ�256�ֽڵ�ʱ��
			data_save();							//��Ҫ�����ݱ���������
			data_count=0;							//ͬʱ��������ͳ�Ʊ�־λ��
		}
		if(proj_count>FLASH_SIZE){					//������յ��Ĵ�С�����ܵĿռ䣬
			send_string("\r\n[Overflow](6)\r\n");	//��ʾ���������
			while(1);								//��ѭ����������Ƭ����
		}
		timeout=50;									//��ʱʱ��Ϊ50*20ms=1000ms=1s
	}     
} 
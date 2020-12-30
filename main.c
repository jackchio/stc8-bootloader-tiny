/*-------------------------------------------头文件-------------------------------------------*/
#include <STC8.H>
#include "intrins.h"		//单片机C语言使用汇编指令的头文件。
/*-------------------------------------------宏定义-------------------------------------------*/
typedef unsigned char u8;	//无符号的 8位变量。
typedef unsigned int  u16;	//无符号的16位变量。
typedef unsigned long u32;	//无符号的32位变量。
/*-----------------------------------------BOOT脚定义-----------------------------------------*/
#define	BOOT_PIN		P00	//BOOT脚设置为P0.0脚。
#define	BOOT_RUN_APP	0	//BOOT脚为低电平的时候运行APP。
/*------------------------------------------功能定义------------------------------------------*/
sfr			IAP_TPS			=0xf5;	//EEPROM擦除等待时间控制寄存器
u8	code	INIT_MASK[6]	={0xff,0xff,0xff,0xff,0xff,0xff};//初始化掩码，对于FLASH而言就是0xFF。
void	(* jmp_app)(void);			//跳转函数。
#define	BOOT_STATUS			0xFE00	//FE00		：BootLoader区状态
#define	BOOT_JMP_ADDR		0xFE01	//FE01~FE02	：BootLoader的跳转地址

#define	APP_START			0xFE03	//FE03		：APP区起始地址
#define	APP_MAX				5		//APP区占用空间为6字节
#define	APP_STATUS			0xFE03	//FE03		：APP区状态
#define	APP_SIZE			0xFE04	//FE04~FE05	：APP区大小
#define	APP_JMP_ADDR		0xFE06	//FE06~FE07	：APP地址

#define	DATA_MAX			124				//APP区的最大值，单位为扇区数。一扇区为512字节。
#define	FLASH_SIZE			(DATA_MAX*512)	//APP区的最大值，单位为字节。

#define	read_jmp__addr		(*(unsigned int  code *)(0x0001))		//读取整个程序的跳转地址。
#define	read_jmp__addr_h	(*(unsigned char code *)(0x0001))
#define	read_jmp__addr_l	(*(unsigned char code *)(0x0002))

#define	read_boot_addr		(*(unsigned int  code *)(BOOT_JMP_ADDR))//读取Bootloader的跳转地址。
#define	read_boot_addr_h	(*(unsigned char code *)(BOOT_JMP_ADDR))
#define	read_boot_addr_l	(*(unsigned char code *)(BOOT_JMP_ADDR+1))

#define	read_app__addr		(*(unsigned int  code *)(APP_JMP_ADDR))	//读取APP的跳转地址。
#define	read_app__addr_h	(*(unsigned char code *)(APP_JMP_ADDR))	
#define	read_app__addr_l	(*(unsigned char code *)(APP_JMP_ADDR+1))

#define	read_app__size		(*(unsigned int  code *)(APP_SIZE))		//读取APP的大小。
#define	read_app__size_h	(*(unsigned char code *)(APP_SIZE))
#define	read_app__size_l	(*(unsigned char code *)(APP_SIZE+1))

#define	read_app__flag		(*(unsigned char code *)(APP_STATUS))	//读取APP的状态。
#define	read_boot_flag		(*(unsigned char code *)(BOOT_STATUS))	//读取BootLoader的状态。
//位定义	[		D7		][			D6			][	D5	][	D4	][	D3	][	D2	][	D1	][	D0	]
//APP	[	使能标志位	][	安装  完成标志位		][						预留					]
//BOOT	[	使能标志位	][	初始化完成标志位		][						预留					]
#define	BIT_ENABLE		0x80					//使能位。
#define	SET_ENABLE		(~(u8)(BIT_ENABLE))		//使能该区域。
#define	GET_ENABLE		( (u8)(BIT_ENABLE))		//查询使能位。

#define	BIT_DONE		0x40					//完成位。
#define	SET_DONE		(~(u8)(BIT_DONE))		//该区域完成了设定功能。
#define	GET_DONE		( (u8)(BIT_DONE))		//查询完成位。

#define	PD_APP__OK		(GET_ENABLE|GET_DONE)	//判断APP区是否有合法的程序。
#define	APP__OK			(0)						//APP区是合法的程序。
/*------------------------------------------函数定义------------------------------------------*/
#if 1//串口部分
/*-------------------------------------------------------
函数名：hard_init
描  述：硬件外设初始化函数。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
2020-12-08：合并了串口和定时器的初始化。更名为hard_init。
-------------------------------------------------------*/
void hard_init(void){	
	SCON = 0x50;		//8位数据,可变波特率
	AUXR = 0x40;		//定时器1时钟为Fosc,即1T
	TMOD = 0x00;		//设定定时器1为16位自动重装方式
	TL0 = 0xC0;			//设置定时初值
	TH0 = 0x63;			//20ms
	TL1 = 0xCC;			//设定初值
	TH1 = 0xFF;			//设定初值
	TF0 = 0;			//清除TF0标志
	TF1 = 0;			//清除TF1标志
	TR0 = 0;
	TR1 = 1;			//启动定时器1
	ET1 = 0;			//禁止定时器1中断
}
/*-------------------------------------------------------
函数名：send_data
描  述：串口发送单字节函数。
输  入：	dat	-	要发送的字节。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
-------------------------------------------------------*/
void send_data(u8 dat){
    SBUF=dat;	//发送一个数据。
	while(!TI);	//等待数据发完。
	TI=0;		//清除发送标志位。
}
/*-------------------------------------------------------
函数名：send_string
描  述：串口发送字符串函数。
输  入：	str	-	要发送的字符串。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
2020-12-08：优化了函数的占用空间。
-------------------------------------------------------*/
void send_string(u8 code * str){
	while(*str){	//判断字符串是否到头，
		SBUF=*str++;//发送一个数据。
		while(!TI);	//等待数据发完。
		TI=0;		//清除发送标志位。
	}	
}
/*-------------------------------------------------------
函数名：send_num
描  述：串口发送16位数据函数，发送格式为%u。
输  入：	dat	-	要发送的数据。
创建者：奈特
调用例程：无
创建日期：2020-12-03
修改记录：
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
#if 1//FLASH操作部分
/*-------------------------------------------------------
函数名：eeprom_off
描  述：eeprom关闭函数。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
-------------------------------------------------------*/
void eeprom_off(void){
    IAP_CONTR = 0;   //关闭IAP功能
    IAP_CMD   = 0;   //清除命令寄存器
    IAP_TRIG  = 0;   //清除触发寄存器
    IAP_ADDRH = 0xff;//将地址设置到非IAP区域
    IAP_ADDRL = 0xff;//将地址设置到非IAP区域
}
/*-------------------------------------------------------
函数名：eeprom_erase
描  述：eeprom擦除函数，用于擦除一个扇区。
输  入：	addr	-	要擦除的地址。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
-------------------------------------------------------*/
void eeprom_erase(u16 addr){
	IAP_CONTR = 0x81;		//使能IAP
	IAP_CMD   = 3;			//设置IAP擦除命令
	IAP_ADDRL = addr;		//设置IAP低地址
	IAP_ADDRH = addr >> 8;	//设置IAP高地址
	IAP_TRIG  = 0x5a;		//写触发命令(0x5a)
	IAP_TRIG  = 0xa5;		//写触发命令(0xa5)
	_nop_();				//稍稍等待一下
	eeprom_off();			//关闭IAP功能
}
/*-------------------------------------------------------
函数名：eeprom_write
描  述：eeprom单字节写函数，用于向指定地址写一个数据。
输  入：	addr	-	要写入的地址；
		dat		-	要写入的数据。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
-------------------------------------------------------*/
void eeprom_write(u16 addr,u8 dat){
	IAP_CONTR = 0x81;		//使能IAP
	IAP_CMD   = 2;			//设置IAP写命令
	IAP_ADDRL = addr;		//设置IAP低地址
	IAP_ADDRH = addr >> 8;	//设置IAP高地址
	IAP_DATA  = dat;		//写IAP数据
	IAP_TRIG  = 0x5a;		//写触发命令(0x5a)
	IAP_TRIG  = 0xa5;		//写触发命令(0xa5)
	_nop_();				//稍稍等待一下
	eeprom_off();			//关闭IAP功能
}
/*-------------------------------------------------------
函数名：eeprom_write_boot_area
描  述：eeprom多字节写系统区函数，用于向指定区域写多个数据。
输  入：	addr	-	要写入的地址，必须要系统区域内；
		buf		-	要写入的数据缓存；
		len		-	要写入的数据数量。
创建者：奈特
调用例程：无
创建日期：2020-11-30
修改记录：
2020-12-03：优化了功能，现在只为固定区域服务已减小占用空间，
			改名为eeprom_write_boot_area。
-------------------------------------------------------*/
volatile u8 xdata eeprom_buf[512];//用于整合数据的缓存
void eeprom_write_boot_area(u16 addr,u8 * buf,u16 len){
	u16 i,ad;
	for(i=0;i<512;i++){				//将系统区原本的数据读出来，放到缓存当中。
		eeprom_buf[i]=(*(unsigned char code *)(BOOT_STATUS+i));
	}
	eeprom_erase(BOOT_STATUS);			//清除系统区。
	ad=addr-BOOT_STATUS;					//获取地址的偏移量。
	for(i=0;i<len;i++){				//将数据写入到缓存。
		eeprom_buf[i+ad]=buf[i];
	}
	for(i=0;i<512;i++){				//将缓存写回到单片机。
		eeprom_write(BOOT_STATUS+i,eeprom_buf[i]);
	}
}
#endif

volatile u16 data_count=0,proj_count=0;//包内数据计数器和总数据计数变量。
/*-------------------------------------------------------
函数名：data_save
描  述：数据保存函数，用于保存从串口接收到的数据。
创建者：奈特
调用例程：无
创建日期：2020-12-24
修改记录：
-------------------------------------------------------*/
void data_save(void){
	u16 i;
	if(proj_count<=256){											//如果总计数小于256，说明是第一包数据。
		eeprom_write(APP_JMP_ADDR  ,eeprom_buf[1]);					//第一包数据通常都包含了跳转地址，
		eeprom_write(APP_JMP_ADDR+1,eeprom_buf[2]);					//把跳转地址保存起来。
		for(i=3;i<proj_count;i++){									//然后写入剩下的数据。
			eeprom_write(i,eeprom_buf[i]);
		}
	}else{															//如果不是第一包数据，
		for(i=0;i<data_count;i++){									//那就直接保存到flash就行了。
			eeprom_write(i+(proj_count-data_count),eeprom_buf[i]);
		}
	}
}

void main(void){
	u16 i;		//16位临时变量。
	u8 timeout;	//时间超时计数器。
	IAP_TPS=24;	//STC8G和STC8H的设置，默认24MHz。
/*------------------------------------------BootLoader初始化-------------------------------*/
	if(read_boot_flag&GET_ENABLE){											//判断是不是第一次运行bootloader，					
		eeprom_write(BOOT_STATUS,		SET_ENABLE);						//是的话就置位使能标志。
		eeprom_write( APP_JMP_ADDR,		(*(unsigned char code *)(0x0001)));	//初始化APP的跳转地址。
		eeprom_write( APP_JMP_ADDR+1,	(*(unsigned char code *)(0x0002)));	//
		eeprom_write(BOOT_JMP_ADDR,		(*(unsigned char code *)(0x0001)));	//初始化BOOT的跳转地址。
		eeprom_write(BOOT_JMP_ADDR+1,	(*(unsigned char code *)(0x0002)));	//
		eeprom_write(BOOT_STATUS,		SET_DONE);							//置位初始化完毕标志。
	}																		//至此BootLoader初始化完成。
/*------------------------------------------APP检测---------------------------------------*/	
	jmp_app=(u32)read_boot_addr;				//默认把BootLoader的地址给APP地址。防止乱跳转。
	if((read_app__flag&PD_APP__OK)==APP__OK){	//判断程序是否合法。
		jmp_app=(u32)read_app__addr;			//程序合法，则将APP跳转地址给跳转函数。
		if(BOOT_PIN==BOOT_RUN_APP){jmp_app();}	//BOOT脚是能跳转到APP的电平就跳转到APP。
	}
/*-----------------------------------BootLoader界面--------------------------------------*/
	hard_init(); 					//初始化串口和定时器。
	send_string("[Boot](0)\r\n");	//提示进入了BOOT状态。
	while(!RI);						//等待接收解锁码。
	RI=0;							//置零接收标志位。
	if(SBUF!='!'){					//如果不是解锁码，说明串口此时是乱传数据的。
		send_string("[Lock](1)\r\n");//提示要锁定单片机。
		while(1);					//死循环，锁定单片机不能继续下载。
	}
/*--------------------------------------下载界面------------------------------------------*/
	send_string("[Erase](2)\r\n");									//提示开始擦除。
	eeprom_erase(0x0000);											//先擦第一扇区。
	eeprom_write(0x0000,0x02);										//立刻写入跳转指令。
	eeprom_write(0x0001,(*(unsigned char code *)(BOOT_JMP_ADDR)));	//取出BOOT的跳转地址高8位，写入到指定地址中。
	eeprom_write(0x0002,(*(unsigned char code *)(BOOT_JMP_ADDR+1)));//取地址低8位，分成两个单字节写函数，可以加快运行速度。
	eeprom_write_boot_area(APP_START,INIT_MASK,APP_MAX);			//初始化APP状态区。
	eeprom_write(APP_STATUS,SET_ENABLE);							//写入APP使能标志位。
	for(i=512;i<FLASH_SIZE;i+=512){									//擦除后面的扇区。
		eeprom_erase(i);
	}
	data_count=0;				//清零单包数据计数器。
	proj_count=0;				//清零总体数据计数器。
	send_string("[Start](3)");	//提示开始下载。
	TR0=1;						//打开定时器，即启用超时判断。
	timeout=250;				//第一次超时时间为250*20ms=5000ms=5s，上位机点击操作需要时间，第一次超时判断要久一些。
	while(1){															//在循环里接收数据。
		while(!RI){														//等待串口来数据。
			if(TF0){													//每当定时器溢出时，
				TF0=0;													//清零标志位。
				timeout--;												//超时时间-1。
				if(timeout==0){											//当时间减到0的时候，说明超时了。
					TR0=0;												//关闭定时器。
					if(proj_count){										//如果期间接收了数据，
						send_string("\r\n[Get ");						//显示接收了多少数据。
						send_num(proj_count);
						send_string(" Byte](4)\r\n");					//单位是字节。
						data_save();									//保存这些数据，在这里保存的通常是最后一包的数据。
						eeprom_write(APP_SIZE,	(u8)(proj_count>>8));	//读取APP的大小高8位，写入到指定地址中。
						eeprom_write(APP_SIZE+1,(u8)(proj_count));		//读取APP的大小低8位，分成两个单字节写函数，可以加快运行速度。
						eeprom_write(APP_STATUS,SET_DONE);				//安装完成，写完成标志位。
						jmp_app=(u32)read_app__addr;					//将APP跳转地址给跳转函数。
					}else{												//如果期间没有接收到数据，
						send_string("\r\n[Time Out](5)\r\n");			//显示超时。
						jmp_app=(u32)read_boot_addr;					//将BOOT的跳转地址给跳转函数。
					}
					jmp_app();											//下载进程完毕，跳转。
				}
			}
		}
		TF0=0;										//如果接收到串口数据，也要清零定时器标志位。
		RI=0;										//同时清零接收标志位。
		eeprom_buf[data_count++]=SBUF;				//将接收的数据保存下来。
		proj_count++;								//统计目前已接收的数据大小。
		if(data_count==256){						//sscom5上位机是256字节分一包，所以每收到256字节的时候，
			data_save();							//就要把数据保存下来。
			data_count=0;							//同时清零数据统计标志位。
		}
		if(proj_count>FLASH_SIZE){					//如果接收到的大小大于总的空间，
			send_string("\r\n[Overflow](6)\r\n");	//提示容量溢出。
			while(1);								//死循环，锁定单片机。
		}
		timeout=50;									//超时时间为50*20ms=1000ms=1s
	}     
} 
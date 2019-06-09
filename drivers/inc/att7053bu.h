/*
 * 文件名称：att7053bu.h
 *
 * 文件说明：功率计模组驱动头文件
 *
 */

#ifndef __ATT7053BU_H__
#define __ATT7053BU_H__

#include "pub_def.h"
#include "error.h"
/********************************************
* Defines *
********************************************/ 
//计量芯片寄存器定义------------------------------------------------------------
#define	SPlI1			0x00			//电流通道1的ADC采样数据
#define	SPlI2			0x01			//电流通道2的ADC采样数据
#define	SPlU			0x02			//电压通道的ADC采样数据
#define	SPlP			0x03			//有功功率波形数据	         *
#define	SPlQ			0x04			//无功功率波形数据	         *
#define	SPlS			0x05			//视在功率波形数据	         *
#define	RMSI1			0x06			//电流通道1的有效值
#define	RMSI2			0x07			//电流通道2的有效值
#define	RMSU			0x08			//电压通道的有效值
#define	FREQU			0x09			//电压频率
#define	POWERP1		    0x0A			//第一通道有功功率
#define	POWERQ1		    0x0B			//第一通道无功功率
#define	PowerS		    0x0C			//视在功率
#define	EnergyP0		0x0D			//有功能量
#define	EnergyQ0		0x0E			//无功能量
#define	EnergyS0		0x0F			//视在能量
#define	POWERP2		    0x10			//第二通道有功功率
#define	POWERQ2		    0x11			//第二通道无功功率
#define	MaxWave		    0x12			//电压波形峰值寄存器
#define	BackupData	    0x16			//通讯数据备份寄存器
#define	ComChkSum		0x17			//通讯校验和寄存器
#define	SumChkSum		0x18			//校表参数校验和寄存器
#define	EMUSR			0x19			//EMU状态寄存器
#define	SYSSTA		    0x1A			//系统状态寄存器
#define	ATT_CHIPID	    0x1B			//芯片ID,默认值ATT7053B0
#define	DEVICEID		0x1C			//器件ID,默认值ATT705301

#define	EMUIE			0x30			//EMU中断使能寄存器
#define	EMUIF			0x31			//EMU中断标志寄存器
#define	WPREG			0x32			//写保护寄存器
#define	SRSTREG		    0x33			//软件复位寄存器

#define	EMUCFG		    0x40			//EMU配置寄存器
#define	FREQCFG		    0x41			//时钟/更新频率配置寄存器
#define	ModuleEn		0x42			//EMU模块使能寄存器
#define	ANAEN			0x43			//模拟模块使能寄存器
#define	STATUSCFG		0x44			//STATUS输出配置寄存器           *
#define	IOCFG			0x45			//IO输出配置寄存器

#define	GP1			    0x50			//通道1的有功功率校正
#define	GQ1			    0x51			//通道1的无功功率校正
#define	GS1			    0x52			//通道1的视在功率校正
#define	Phase1		    0x53			//通道1的相位校正（移采样点方式）*
#define	GP2			    0x54			//通道2的有功功率校正
#define	GQ2			    0x55			//通道2的无功功率校正
#define	GS2			    0x56			//通道2的视在功率校正
#define	Phase2		    0x57			//通道2的相位校正（移采样点方式）*
#define	QPhsCal		    0x58			//无功相位补偿
#define	ADCCON		    0x59			//ADC通道增益选择
#define	AllGain		    0x5A			//3个ADC通道整体增益寄存器       *
#define	I2Gain		    0x5B			//电流通道2增益补偿
#define	I1Off			0x5C			//电流通道1的偏置校正
#define	I2Off			0x5D			//电流通道2的偏置校正
#define	UOff			0x5E			//电压通道的偏置校正
#define	PQStart		    0x5F			//启动功率设置
#define	RMSStart		0x60			//有效值启动值设置               *
#define	HFConst		    0x61			//输出脉冲频率设置
#define	CHK			    0x62			//窃电阈值设置
#define	IPTEMP		    0x63			//窃电检测阈值设置
#define	UConst		    0x64			//电压有效值替代值设置*
#define	P1Offset		0x65			//通道1有功功率偏置校正参数
#define	P2Offset		0x66			//通道2有功功率偏置校正参数
#define	Q1Offset		0x67			//通道1无功功率偏置校正参数
#define	Q2Offset		0x68			//通道2无功功率偏置校正参数
#define	I1RMSOff		0x69			//电流通道1有效值补偿寄存器
#define	I2RMSOff		0x6A			//电流通道2有效值补偿寄存器
#define	URMSOff		    0x6B			//电压通道有效值补偿寄存器*
#define	ZCrossCur		0x6C			//电流过零阈值设置寄存器
#define	GPhs1			0x6D			//通道1的相位校正（PQ方式）
#define	GPhs2			0x6E			//通道2的相位校正（PQ方式）
#define	PFCnt			0x6F			//快速有功脉冲计数
#define	QFCnt			0x70			//快速无功脉冲计数
#define	SFCnt			0x71			//快速视在脉冲计数

#define	ANACON		    0x72			//模拟控制寄存器
#define	SumChkL		    0x73			//校验和低16位
#define	SumChkH		    0x74			//校验和高8位
#define	MODECFG		    0x75			//模式配置寄存器
#define	P1OffsetL		0x76			//通道1有功功率偏置校正低8位
#define	P2OffsetL		0x77			//通道2有功功率偏置校正低8位
#define	Q1OffsetL		0x78			//通道1无功功率偏置校正低8位
#define	Q2OffsetL		0x79			//通道2无功功率偏置校正低8位
#define	UPeakLvl		0x7A			//UPEAK阈值寄存器
#define	USagLvl		    0x7B			//USAG阈值寄存器
#define	UCycLen		    0x7C			//PEAK&SAG检测周期设置寄存器

/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/


/********************************************
* Function *
********************************************/
error_t att7053buInit(void);

uint32_t att7053buPowerGet(void);
    
#endif    
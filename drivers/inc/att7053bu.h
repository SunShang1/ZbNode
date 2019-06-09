/*
 * �ļ����ƣ�att7053bu.h
 *
 * �ļ�˵�������ʼ�ģ������ͷ�ļ�
 *
 */

#ifndef __ATT7053BU_H__
#define __ATT7053BU_H__

#include "pub_def.h"
#include "error.h"
/********************************************
* Defines *
********************************************/ 
//����оƬ�Ĵ�������------------------------------------------------------------
#define	SPlI1			0x00			//����ͨ��1��ADC��������
#define	SPlI2			0x01			//����ͨ��2��ADC��������
#define	SPlU			0x02			//��ѹͨ����ADC��������
#define	SPlP			0x03			//�й����ʲ�������	         *
#define	SPlQ			0x04			//�޹����ʲ�������	         *
#define	SPlS			0x05			//���ڹ��ʲ�������	         *
#define	RMSI1			0x06			//����ͨ��1����Чֵ
#define	RMSI2			0x07			//����ͨ��2����Чֵ
#define	RMSU			0x08			//��ѹͨ������Чֵ
#define	FREQU			0x09			//��ѹƵ��
#define	POWERP1		    0x0A			//��һͨ���й�����
#define	POWERQ1		    0x0B			//��һͨ���޹�����
#define	PowerS		    0x0C			//���ڹ���
#define	EnergyP0		0x0D			//�й�����
#define	EnergyQ0		0x0E			//�޹�����
#define	EnergyS0		0x0F			//��������
#define	POWERP2		    0x10			//�ڶ�ͨ���й�����
#define	POWERQ2		    0x11			//�ڶ�ͨ���޹�����
#define	MaxWave		    0x12			//��ѹ���η�ֵ�Ĵ���
#define	BackupData	    0x16			//ͨѶ���ݱ��ݼĴ���
#define	ComChkSum		0x17			//ͨѶУ��ͼĴ���
#define	SumChkSum		0x18			//У�����У��ͼĴ���
#define	EMUSR			0x19			//EMU״̬�Ĵ���
#define	SYSSTA		    0x1A			//ϵͳ״̬�Ĵ���
#define	ATT_CHIPID	    0x1B			//оƬID,Ĭ��ֵATT7053B0
#define	DEVICEID		0x1C			//����ID,Ĭ��ֵATT705301

#define	EMUIE			0x30			//EMU�ж�ʹ�ܼĴ���
#define	EMUIF			0x31			//EMU�жϱ�־�Ĵ���
#define	WPREG			0x32			//д�����Ĵ���
#define	SRSTREG		    0x33			//�����λ�Ĵ���

#define	EMUCFG		    0x40			//EMU���üĴ���
#define	FREQCFG		    0x41			//ʱ��/����Ƶ�����üĴ���
#define	ModuleEn		0x42			//EMUģ��ʹ�ܼĴ���
#define	ANAEN			0x43			//ģ��ģ��ʹ�ܼĴ���
#define	STATUSCFG		0x44			//STATUS������üĴ���           *
#define	IOCFG			0x45			//IO������üĴ���

#define	GP1			    0x50			//ͨ��1���й�����У��
#define	GQ1			    0x51			//ͨ��1���޹�����У��
#define	GS1			    0x52			//ͨ��1�����ڹ���У��
#define	Phase1		    0x53			//ͨ��1����λУ�����Ʋ����㷽ʽ��*
#define	GP2			    0x54			//ͨ��2���й�����У��
#define	GQ2			    0x55			//ͨ��2���޹�����У��
#define	GS2			    0x56			//ͨ��2�����ڹ���У��
#define	Phase2		    0x57			//ͨ��2����λУ�����Ʋ����㷽ʽ��*
#define	QPhsCal		    0x58			//�޹���λ����
#define	ADCCON		    0x59			//ADCͨ������ѡ��
#define	AllGain		    0x5A			//3��ADCͨ����������Ĵ���       *
#define	I2Gain		    0x5B			//����ͨ��2���油��
#define	I1Off			0x5C			//����ͨ��1��ƫ��У��
#define	I2Off			0x5D			//����ͨ��2��ƫ��У��
#define	UOff			0x5E			//��ѹͨ����ƫ��У��
#define	PQStart		    0x5F			//������������
#define	RMSStart		0x60			//��Чֵ����ֵ����               *
#define	HFConst		    0x61			//�������Ƶ������
#define	CHK			    0x62			//�Ե���ֵ����
#define	IPTEMP		    0x63			//�Ե�����ֵ����
#define	UConst		    0x64			//��ѹ��Чֵ���ֵ����*
#define	P1Offset		0x65			//ͨ��1�й�����ƫ��У������
#define	P2Offset		0x66			//ͨ��2�й�����ƫ��У������
#define	Q1Offset		0x67			//ͨ��1�޹�����ƫ��У������
#define	Q2Offset		0x68			//ͨ��2�޹�����ƫ��У������
#define	I1RMSOff		0x69			//����ͨ��1��Чֵ�����Ĵ���
#define	I2RMSOff		0x6A			//����ͨ��2��Чֵ�����Ĵ���
#define	URMSOff		    0x6B			//��ѹͨ����Чֵ�����Ĵ���*
#define	ZCrossCur		0x6C			//����������ֵ���üĴ���
#define	GPhs1			0x6D			//ͨ��1����λУ����PQ��ʽ��
#define	GPhs2			0x6E			//ͨ��2����λУ����PQ��ʽ��
#define	PFCnt			0x6F			//�����й��������
#define	QFCnt			0x70			//�����޹��������
#define	SFCnt			0x71			//���������������

#define	ANACON		    0x72			//ģ����ƼĴ���
#define	SumChkL		    0x73			//У��͵�16λ
#define	SumChkH		    0x74			//У��͸�8λ
#define	MODECFG		    0x75			//ģʽ���üĴ���
#define	P1OffsetL		0x76			//ͨ��1�й�����ƫ��У����8λ
#define	P2OffsetL		0x77			//ͨ��2�й�����ƫ��У����8λ
#define	Q1OffsetL		0x78			//ͨ��1�޹�����ƫ��У����8λ
#define	Q2OffsetL		0x79			//ͨ��2�޹�����ƫ��У����8λ
#define	UPeakLvl		0x7A			//UPEAK��ֵ�Ĵ���
#define	USagLvl		    0x7B			//USAG��ֵ�Ĵ���
#define	UCycLen		    0x7C			//PEAK&SAG����������üĴ���

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
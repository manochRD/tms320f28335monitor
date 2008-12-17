//###########################################################################
//
// FILE:   flash.c
//
// TITLE:  SSM_ DSPMATE  28335 Monitor Flash
//
// DESCRIPTION:
//	    				
//         Jeon yu hun  
//
//###########################################################################
// $Release Date: 11-25, 2008 $
//###########################################################################

#include "DSP2833x_Device.h"     // DSP2833x Headerfile Include File
#include "DSP2833x_Examples.h"   // DSP2833x Examples Include File

#define SCI		0
#define FLASH	1

/*
   FLASHH      : origin = 0x300000, length = 0x008000    <- User program
   FLASHG      : origin = 0x308000, length = 0x008000    
   FLASHF      : origin = 0x310000, length = 0x008000     
   FLASHE      : origin = 0x318000, length = 0x008000     
   FLASHD      : origin = 0x320000, length = 0x008000     
   FLASHC      : origin = 0x328000, length = 0x008000     
   FLASHB      : origin = 0x330000, length = 0x008000     
   FLASHA      : origin = 0x338000, length = 0x007F80     <- Monitor
*/
#define USER_FLASH	(Uint32)0x300000

//External RAM zone6
#define USER_RAM	(Uint32)0x100000

#define FLASH_DEBUG		0

const Uint16 FlashSector[8] = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};

Uint16 *pFlashAdd;
Uint16 *pRamAdd;

#define USER_PRG_ADD	(Uint32)0x008300  // user program code_start

//#pragma CODE_SECTION(Convert_HEX_AtoI, "ramfuncs");
//#pragma CODE_SECTION(DownHEXFrom, "ramfuncs");
//#pragma CODE_SECTION(DownUserProgfrom, "ramfuncs");
//#pragma CODE_SECTION(FlashBurnPrm, "ramfuncs");


/*--- Global variables used to interface to the flash routines */
FLASH_ST FlashStatus;
volatile struct HEX_FIELD DownLoadingHex;

void InitFlashAPI(void)
{

/*
      5. If required, copy the flash API functions into on-chip zero waitstate
         RAM.  
      6. Initalize the Flash_CPUScaleFactor variable to SCALE_FACTOR
      7. Initalize the callback function pointer or set it to NULL
      8. Optional: Run the Toggle test to confirm proper frequency configuration
         of the API. 
      9. Optional: Unlock the CSM.
     10. Make sure the PLL is not running in limp mode  
*/
	float32 Version;        // Version of the API in floating point
	Uint16 VersionHex;     // Version of the API in decimal encoded hex
	Uint16 flash_Status;

	MemCopy(&Flash28_API_LoadStart, &Flash28_API_LoadEnd, &Flash28_API_RunStart);


	Flash_CPUScaleFactor = SCALE_FACTOR;
	Flash_CallbackPtr = NULL; 

/*------------------------------------------------------------------
 Unlock the CSM.
    If the API functions are going to run in unsecured RAM
    then the CSM must be unlocked in order for the flash 
    API functions to access the flash.
   
    If the flash API functions are executed from secure memory 
    (L0-L3) then this step is not required.
------------------------------------------------------------------*/

	flash_Status = CsmUnlock();
	if(flash_Status != STATUS_SUCCESS) 
	{
		TxPrintf("\n  Flash Csmunlock Error!!\n");
		asm("    ESTOP0");
	}


	//Version check
	VersionHex = Flash_APIVersionHex();
	if(VersionHex != 0x0210)
	{
		// Unexpected API version
		// Make a decision based on this info. 
		TxPrintf("\n  Flash API version Error!!\n");
		asm("    ESTOP0");
	}   


	Version = Flash_APIVersion();
	if(Version != (float32)2.10)
	{
		// Unexpected API version
		// Make a decision based on this info. 
		TxPrintf("\n  Flash API version Error!!\n");
		asm("    ESTOP0");
	}
   
	
}

/*   INITSTRUCT_HEXDOWN   */
/*************************************************************************
*	@name    	InitStruct_HexDown
*	@memo	       HEX �ǻ��¿� ���� ��巹���� HEX �ٿ�ε���� ����ü ZERO �ʱ�ȭ
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void InitStruct_HexDown(void)
{
	memset((void *)&DownLoadingHex, 0x00, sizeof(DownLoadingHex));
}

/*   ERASE_SELECTFLASH   */
/*************************************************************************
*	@name    	Erase_SelectFlash
*	@memo	       ���õ� FLASH ������ �����.
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void Erase_SelectFlash(void)
{
	Uint16 Status;
	char RcvData;
	Uint16 EraseSector;

	TxPrintf("\nErase sector... Select Sector(B~H) :");

	RcvData = SCIa_RxChar();
	SCIa_TxChar(RcvData);
	switch(RcvData)
	{
		case 'b':
		case 'B':
			EraseSector = SECTORB;
			TxPrintf("\n  Erase SectorB.\n");
			break;
			
		case 'c':
		case 'C':
			EraseSector = SECTORC;
			TxPrintf("\n  Erase SectorC.\n");
			break;
			
		case 'd':
		case 'D':
			EraseSector = SECTORD;
			TxPrintf("\n  Erase SectorD.\n");
			break;

		case 'e':
		case 'E':
			EraseSector = SECTORE;
			TxPrintf("\n  Erase SectorE.\n");
			break;

		case 'f':
		case 'F':
			EraseSector = SECTORF;
			TxPrintf("\n  Erase SectorF.\n");
			break;

		case 'g':
		case 'G':
			EraseSector = SECTORG;
			TxPrintf("\n  Erase SectorG.\n");
			break;

		case 'h':
		case 'H':
			EraseSector = SECTORH;
			TxPrintf("\n  Erase SectorH.\n");
			break;
			
		default:
			TxPrintf("\nWrong Sector Selected Type B to H\n");
			return;
	}

	Status = Flash_Erase(EraseSector, &FlashStatus);

	if(Status != STATUS_SUCCESS)
	{
		TxPrintf("\n  Flash_Erase Error!!\n");
		return;
	}

	TxPrintf("\n  Erase Sector %c OK!!\n",RcvData);

	
}

/*   ERASE_ALLFLASH   */
/*************************************************************************
*	@name    	Erase_AllFlash
*	@memo	       ��� FLASH������ �����. SectorA �� ��������α׷��̶� ����
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void Erase_AllFlash(void)
{
	Uint16 Status;

	TxPrintf("\n  Erase All Flash Sector.\n");
				
	Status = Flash_Erase(SECTORB|SECTORC|SECTORD|SECTORE|SECTORF|SECTORG|SECTORH, &FlashStatus);
	
	if(Status != STATUS_SUCCESS)
	{
		TxPrintf("\n  Flash_Erase Error!!\n");
		return;
	}
	
	TxPrintf("\n  Erase All Flash Sector OK!!(SECTOR B ~ SECTOR H)\n");
	
}

/*   DOWNFROMSCI   */
/*************************************************************************
*	@name    	DownFromSCI
*	@memo	       �������� ���� �������α׷� �ٿ�ε�
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void DownFromSCI(void)
{
	InitStruct_HexDown();
	TxPrintf("\n  Send User Program *.Hex\n");

	SCIa_TxChar(BEL);

	if(DownUserProgfrom(SCI)) 
		TxPrintf("\n  DownLoading Success !!");
	else 
	{
		TxPrintf("\n  DownLoading Failure !!"); 
		return;
	}
	TxPrintf("\n  Go To User Program !!\n");

	Go_UserProgram();
	
}

void FlashtoRamDownloadPrm(void)
{
	SetUserHEXFlashadd();
	InitStruct_HexDown();
	
	if(DownUserProgfrom(FLASH)) 
		TxPrintf("\n  DownLoading Success !!");
	else 
	{
		TxPrintf("\n  DownLoading Failure !!"); 
		//return;//
	}

	TxPrintf("\n  Go To User Program !!\n");

	Go_UserProgram();
		
}

void FlashBurnPrm(void)
{
	char RcvData[2];
	Uint16 BurnData;
	Uint16 Status;
	Uint16 Buf[15] = {0,};		
	Uint16 EndCnt = 0;

	Uint32 BurnCnt;
	Uint32 i;
	
	TxPrintf("\n  Send User Program *.Hex\n");
	
	SCIa_TxChar(BEL);
	
	pFlashAdd = (Uint16 *)USER_FLASH;
	pRamAdd = (Uint16 *)USER_RAM;
	BurnCnt = 0;

	for(;;)
	{
		RcvData[0] = SCIa_RxChar();
		RcvData[1] = SCIa_RxChar();

		BurnData = ((RcvData[0] << 8) + RcvData[1]);

		*(pRamAdd + BurnCnt++) = BurnData;

		if(RcvData[0] == ':')
		{
			SCIa_TxChar('.');
			EndCnt = 0;
			Buf[EndCnt++] = RcvData[0];
			Buf[EndCnt++] = RcvData[1];
		}
		else if(RcvData[1] == ':')
		{
			SCIa_TxChar('.');
			EndCnt = 0;
			Buf[EndCnt++] = RcvData[1];
		}
		else
		{
			if(EndCnt > 13)
				EndCnt = 0;
			
			Buf[EndCnt++] = RcvData[0];
			Buf[EndCnt++] = RcvData[1];

			//end of file
			// :00000001FF
			if(Buf[0] == ':')
			if(Buf[1] == '0')
			if(Buf[2] == '0')
			if(Buf[3] == '0')
			if(Buf[4] == '0')
			if(Buf[5] == '0')
			if(Buf[6] == '0')
			if(Buf[7] == '0')			
			if(Buf[8] == '1')
			if(Buf[9] == 'F')
			if(Buf[10] == 'F')
				
				break;
		}

	
	}

	TxPrintf("\n  %ld word downloded!!\n", BurnCnt);
	

	for(i = 1; i < 8; i++)
	{
		if(BurnCnt < ((Uint32)0x8000*i - 0x100))
		{
			TxPrintf("\n  Erase Flash Sector !!\n");
			Status = Flash_Erase(FlashSector[i-1], &FlashStatus);
			if(Status != STATUS_SUCCESS)
			{
				TxPrintf("\n  Flash Error!!\n");
				return;
			}
			else
			{
				TxPrintf("\n  Flash Sector Cnt : %ld Erased !!\n", i);
				i = 0;
				break;
			}
		}
	}
	if(i != 0)
	{
		TxPrintf("\n  User Program Size too Big !!\n");
		return;
	}


	for(i = 0; i < BurnCnt; i++)
	{
		Status = Flash_Program(pFlashAdd++,pRamAdd++,1,&FlashStatus);
		if(Status != STATUS_SUCCESS)
		{
			TxPrintf("\n  Flash Error!!\n");
			return;
		}	
	}

	TxPrintf("\n  Burn User Program End!!\n");
	
	SCIa_TxChar(BEL);
	SCIa_TxChar(BEL);

#if FLASH_DEBUG

	pFlashAdd = (Uint16 *)USER_FLASH;

	for(i = 0; i < BurnCnt; i++)
	{
		RcvData[0] = *(pFlashAdd++);

		Buf[0] = RcvData[0] >> 8;

		if((char)Buf[0] == CR)
		{
			SCIa_TxChar('\r');
			SCIa_TxChar('\n');
		}
		else
			SCIa_TxChar(Buf[0]);
		
		Buf[1] = RcvData[0] & 0x00ff;
		
		if((char)Buf[1] == CR)
		{
			SCIa_TxChar('\r');
			SCIa_TxChar('\n');
		}
		else
			SCIa_TxChar(Buf[1]);
		
	}
#endif
	

}


/*   GO_USERPROGRAM   */
/*************************************************************************
*	@name    	Go_UserProgram
*	@memo	       ����� ���α׷��� �����ּҷ� �����Ѵ�.
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void Go_UserProgram(void)
{
	Uint32 Add = USER_PRG_ADD;   

	void (*UserPrg)(void) = (void (*)(void))Add;
//	void (*UserPrg)(void) = (*(void)Add);

	DINT;

	UserPrg();
	 // ���� ���α׷����� �̵� �� ���� �޸� ���α׷� ��ü �ʱ�ȭ��
}



/*   SETUSERHEXFLASHADD   */
/*************************************************************************
*	@name    	SetUserHEXFlashadd
*	@memo	       user ���α׷� hex�� ����Ǿ��ִ� flash �ּ� ����
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
void SetUserHEXFlashadd(void)
{
	pFlashAdd = (Uint16 *)USER_FLASH;
}


/*   LOADFLASHDATA   */
/*************************************************************************
*	@name    	LoadFlashData
*	@memo	       �÷��ÿ� ����� ���� hex ����Ÿ�� �����´�
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
Uint16 LoadFlashData(void)
{
	static Uint16 Flag = OFF;
	Uint16 Data;

	if(Flag == OFF)
	{
		Data = (*pFlashAdd >> 8) & 0xff;
		Flag = ON;
	}
	else
	{
		Data = *pFlashAdd & 0xff;
		Flag = OFF;
		pFlashAdd++;
	}

	return Data;
}


/*   DOWNUSERPROGFROM   */
/*************************************************************************
*	@name    	DownUserProgfrom
*	@memo	       �������α׷��� �޾Ƽ� ���ġ ��Ų��
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
Uint16 DownUserProgfrom(Uint16 Source)
{
	int i;
	Uint16 CheckSum;
	Uint16 DataBuffer;
	while(DownLoadingHex.Status.bit.end_of_file==FALSE)
	{  
		if(Source == SCI)
		{
			while( SCIa_RxChar() != ':' );
		}
		else//FLASH
		{
			while((i = LoadFlashData()) != ':')
			{
				if(i == 'F')
				{
					TxPrintf("\n  Flash Invalid!! \n");
					return FALSE;
				}
			}
		}
		
		DownLoadingHex.Checksum = CheckSum = 0;
		
		 // hex ������ datalength
		DownLoadingHex.DataLength = DownHEXFrom(2, Source);
		 // 32��Ʈ ��巹�� ���� 16��Ʈ ����
		DownLoadingHex.Address.Word.low16 = DownHEXFrom(4, Source);
		// Data Type
		DownLoadingHex.RecordType = DownHEXFrom(2, Source);

		switch (DownLoadingHex.RecordType) 
		{

	/*
	'00'       Data Record
	'0l'        End of File Record
	'02'       Extended Segment Address Record
	'03'       Start Segment Address Record
	'04'       Extended Linear Address Record
	'05'       Start Linear Address Record
	*/

			// '00'       Data Record
			case 0x00:
				// data lenth *2 = datalenth in byte
				for( i = 0; i < DownLoadingHex.DataLength; i += 2)
				{
					if( ( DownLoadingHex.DataLength - i ) == 1 )  //�̷���찡 ����..
					{
						DataBuffer = DownHEXFrom(2, Source);
						SCIa_TxString("\nHEX converting Error. it is NOT a 16bit data hex\n");
					}
					else 
						DataBuffer = DownHEXFrom(4, Source); // 16 bit hex

						
					 //2008/12/17    ���� �޸𸮿� ����Ÿ ���ġ
					*(Uint16 *)DownLoadingHex.Address.all = DataBuffer;
					DownLoadingHex.Address.all ++;
				}
				break;

			
			// '01'        End of File Record
			case 0x01:
				DownLoadingHex.Status.bit.end_of_file = TRUE;
				break;

				
			// '02'       Extended Segment Address Record	
			case 0x02: //not use
				break;
			//'03'       Start Segment Address Record	
			case 0x03: //not use
				break;

			//      Extended Linear Address 
			case 0x04: //use in 32bit addressing
				 //���� 16��Ʈ ������
				DownLoadingHex.Address.Word.high16 = DownHEXFrom(4, Source);
				break;

			// '05'       Start Linear Address Record
			case 0x05: //not use
				break;

		}
		CheckSum = ((~(DownLoadingHex.Checksum)) + 1) & 0xff; // 2' complement

		if( CheckSum != DownHEXFrom(2, Source) )
		{
			SCIa_TxString("\nCheckSumError");
			return FALSE;
		}

		 SCIa_TxChar('.'); //Dot per one HEX line
	}

	return TRUE;
}


/*   DOWNHEXFROM   */
/*************************************************************************
*	@name    	DownHEXFrom
*	@memo	       Source�� �� Intel hex �ؼ��� ����
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
Uint16 DownHEXFrom( Uint16 NumByte, Uint16 Source)
{
	Uint16 Value, i, Shift_Count;
	char Rcvdata;

	Value = 0;

	Shift_Count = NumByte;	//2008/12/17    16bit ����Ÿ ���翡�� �׻� 2�� 4

	for ( i = 0; i < NumByte; i++ ) 
	{
		if(Source == SCI)
	        	Rcvdata = SCIa_RxChar();
		else
			Rcvdata = LoadFlashData();
		
		Value |= (Uint16)(Convert_HEX_AtoI( Rcvdata ) << ((--Shift_Count) * 4));
//		Value |= Convert_HEX_AtoI( Rcvdata ) << ( ( (NumByte -1)-i ) * 4 );
	}

	 //check sum �κ�
	if( NumByte == 4 )
	{
	    DownLoadingHex.Checksum += (( Value >> 8 ) & 0xff); 
	    DownLoadingHex.Checksum += (Value & 0xff);
	}
	else  // 2����Ʈ�� ���
	{
	    DownLoadingHex.Checksum += (Value & 0xff);
	}

	return Value;
}


/*   CONVERT_HEX_ATOI   */
/*************************************************************************
*	@name    	Convert_HEX_AtoI
*	@memo	       ���� HEX�� ���������� ��ȯ�Ѵ�.
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
char Convert_HEX_AtoI(char Ascii)
{
	if(Ascii >= '0' && Ascii <= '9')return(Ascii-'0');
	else if(Ascii >= 'a' && Ascii<= 'f')return(Ascii-'a'+10);
	else if(Ascii >= 'A' && Ascii <= 'F')return(Ascii-'A'+10);
	else return(0xFF);
}


/*   CSMUNLOCK   */
/*************************************************************************
*	@name    	CsmUnlock
*	@memo	       Flash API�� L0~L2 ������ ������ ���۵ɰ�� CSM �� Ǯ��� �Ѵ�.
*	@author	       Joen yu hun
*	@company	SSM
*    
**************************************************************************/
Uint16 CsmUnlock(void)
{
    volatile Uint16 temp;
    
    // Load the key registers with the current password
    // These are defined in Example_Flash2833x_CsmKeys.asm
    
    EALLOW;
    CsmRegs.KEY0 = PRG_key0;
    CsmRegs.KEY1 = PRG_key1;
    CsmRegs.KEY2 = PRG_key2;
    CsmRegs.KEY3 = PRG_key3;
    CsmRegs.KEY4 = PRG_key4;
    CsmRegs.KEY5 = PRG_key5;
    CsmRegs.KEY6 = PRG_key6;
    CsmRegs.KEY7 = PRG_key7;   
    EDIS;

    // Perform a dummy read of the password locations
    // if they match the key values, the CSM will unlock 
        
    temp = CsmPwl.PSWD0;
    temp = CsmPwl.PSWD1;
    temp = CsmPwl.PSWD2;
    temp = CsmPwl.PSWD3;
    temp = CsmPwl.PSWD4;
    temp = CsmPwl.PSWD5;
    temp = CsmPwl.PSWD6;
    temp = CsmPwl.PSWD7;

    // If the CSM unlocked, return succes, otherwise return
    // failure.
    if ( (CsmRegs.CSMSCR.all & 0x0001) == 0) return STATUS_SUCCESS;
    else return STATUS_FAIL_CSM_LOCKED;
    
}


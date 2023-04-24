/*
*********************************************************************************************************
*
*	Ä£¿éÃû³Æ : XYZmodemÐ­Òé
*	ÎÄ¼þÃû³Æ : xyzmodem.c
*	°æ    ±¾ : V1.0
*	Ëµ    Ã÷ : xyzmodemÐ­Òé
*
*	ÐÞ¸Ä¼ÇÂ¼ 
*		°æ±¾ºÅ    ÈÕÆÚ         ×÷Õß        ËµÃ÷
*		V1.0    2022-08-08  Eric2013      Ê×·¢
*
*	Copyright (C), 2022-2030, °²¸»À³µç×Ó www.armbbs.cn
*
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
//#include <windows.h>
#include "uart_to_mcu.h"
#include <unistd.h>
#include <string.h>

/*
*********************************************************************************************************
*	                                   YmodemÎÄ¼þ´«ÊäÐ­Òé½éÉÜ
*********************************************************************************************************
*/
/*
µÚ1½×¶Î£º Í¬²½
    ´Ó»ú¸øÊý¾Ý·¢ËÍÍ¬²½×Ö·û C
    
µÚ2½×¶Î£º·¢ËÍµÚ1Ö¡Êý¾Ý£¬°üº¬ÎÄ¼þÃûºÍÎÄ¼þ´óÐ¡
    Ö÷»ú·¢ËÍ£º
    ---------------------------------------------------------------------------------------
    | SOH  |  ÐòºÅ - 0x00 |  ÐòºÅÈ¡·´ - 0xff | 128×Ö½ÚÊý¾Ý£¬º¬ÎÄ¼þÃûºÍÎÄ¼þ´óÐ¡×Ö·û´®|CRC0 CRC1|
    |-------------------------------------------------------------------------------------|  
	´Ó»ú½ÓÊÕ£º
    ½ÓÊÕ³É¹¦»Ø¸´ACKºÍCRC16£¬½ÓÊÕÊ§°Ü£¨Ð£Ñé´íÎó£¬ÐòºÅÓÐÎó£©¼ÌÐø»Ø¸´×Ö·ûC£¬³¬¹ýÒ»¶¨´íÎó´ÎÊý£¬»Ø¸´Á½¸öCA£¬ÖÕÖ¹´«Êä¡£

µÚ3½×¶Î£ºÊý¾Ý´«Êä
    Ö÷»ú·¢ËÍ£º
    ---------------------------------------------------------------------------------------
    | SOH/STX  |  ´Ó0x01¿ªÊ¼ÐòºÅ  |  ÐòºÅÈ¡·´ | 128×Ö½Ú»òÕß1024×Ö½Ú                |CRC0 CRC1|
    |-------------------------------------------------------------------------------------|  
	´Ó»ú½ÓÊÕ£º
    ½ÓÊÕ³É¹¦»Ø¸´ACK£¬½ÓÊÕÊ§°Ü£¨Ð£Ñé´íÎó£¬ÐòºÅÓÐÎó£©»òÕßÓÃ»§´¦ÀíÊ§°Ü¼ÌÐø»Ø¸´×Ö·ûC£¬³¬¹ýÒ»¶¨´íÎó´ÎÊý£¬»Ø¸´Á½¸öCA£¬ÖÕÖ¹´«Êä¡£

µÚ4½×¶Î£º½áÊøÖ¡
    Ö÷»ú·¢ËÍ£º·¢ËÍEOT½áÊø´«Êä¡£
	´Ó»ú½ÓÊÕ£º»Ø¸´ACK¡£

µÚ5½×¶Î£º¿ÕÖ¡£¬½áÊøÍ¨»°
    Ö÷»ú·¢ËÍ£ºÒ»Ö¡¿ÕÊý¾Ý¡£
	´Ó»ú½ÓÊÕ£º»Ø¸´ACK¡£
*/

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (64)   //ÎÄ¼þÃû³¤¶È×î¶à64×Ö½Ú£¬°üÀ¨½áÊø·û
#define FILE_SIZE_LENGTH        (16)
#define FILE_MD5_LENGTH         (32)   //MD5³¤¶È32×Ö½Ú£¬²»°üÀ¨½áÊø·û£¬2023-04-12Ôö¼Ó

#define NAK_TIMEOUT             (0x100000)
#define MAX_ERRORS              (5)

extern char md5_readBuf[64];   //´æ·ÅÎÄ¼þµÄmd5


/*
*********************************************************************************************************
*	º¯ Êý Ãû: Int2Str
*	¹¦ÄÜËµÃ÷: ½«ÕûÊý×ª»»³É×Ö·û
*	ÐÎ    ²Î: str ×Ö·û  intnum ÕûÊý
*	·µ »Ø Öµ: ÎÞ
*********************************************************************************************************
*/
static int Int2Str(uint8_t* str,uint32_t arrlen, int32_t intnum)
{
	return snprintf(str,arrlen-1,"%d",intnum);

	// for (i = 0; i < 10; i++)
	// {
	// 	str[j++] = (intnum / Div) + 48;

	// 	intnum = intnum % Div;
	// 	Div /= 10;
	// 	if ((str[j-1] == '0') & (Status == 0))
	// 	{
	// 		j = 0;
	// 	}
	// 	else
	// 	{
	// 		Status++;
	// 	}
	// }
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: Ymodem_PrepareIntialPacket
*	¹¦ÄÜËµÃ÷: ×¼±¸µÚÒ»°üÒª·¢ËÍµÄÊý¾Ý     
*	ÐÎ    ²Î: data Êý¾Ý
*             fileName ÎÄ¼þÃû
*             length   ÎÄ¼þ´óÐ¡
*	·µ »Ø Öµ: 0
*********************************************************************************************************
*/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
	uint16_t i, j;
	uint8_t file_ptr[10];

	/* µÚÒ»°üÊý¾ÝµÄÇ°Èý¸ö×Ö·û  */
	data[0] = SOH; /* soh±íÊ¾Êý¾Ý°üÊÇ128×Ö½Ú */
	data[1] = 0x00;
	data[2] = 0xff;

	/* ÎÄ¼þÃû */
	for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH); i++)
	{
		data[i + PACKET_HEADER] = fileName[i];
	}
//	printf("i+PACKET_HEADER = %d\n",i + PACKET_HEADER);
	data[i + PACKET_HEADER] = 0x00;

	/* ÎÄ¼þ´óÐ¡×ª»»³É×Ö·û */
	//Int2Str (file_ptr, *length);
	snprintf(file_ptr,sizeof file_ptr-1,"%d",*length);
//	printf("file_ptr = %s,len = %ld\n",file_ptr,strlen(file_ptr));
	for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
	{
		data[i++] = file_ptr[j++];
	}

	data[i] = 0x00;
//	printf("i  = %d\n",i );

	for (j =0, i = i + 1; (md5_readBuf[j] != '\0') && (j < FILE_MD5_LENGTH) ;i++,j++ )
	{
		data[i] = md5_readBuf[j];
//		printf("data[%d] = %#x\n",i,data[i]);
	}
//	printf("i = %d, j = %d\n",i,j);
	/* ÆäÓà²¹0 */
	for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
	{
		data[j] = 0;
	}

	//printf("Ymodem_PrepareIntialPacket done\n");
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: Ymodem_PreparePacket
*	¹¦ÄÜËµÃ÷: ×¼±¸·¢ËÍÊý¾Ý°ü    
*	ÐÎ    ²Î: SourceBuf Òª·¢ËÍµÄÔ­Êý¾Ý
*             data      ×îÖÕÒª·¢ËÍµÄÊý¾Ý°ü£¬ÒÑ¾­°üº¬µÄÍ·ÎÄ¼þºÍÔ­Êý¾Ý
*             pktNo     Êý¾Ý°üÐòºÅ
*             sizeBlk   Òª·¢ËÍÊý¾ÝÊý
*	·µ »Ø Öµ: ÎÞ
*********************************************************************************************************
*/
static int sendsize = 0;
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
	uint16_t i, size, packetSize;
	uint8_t* file_ptr;

	/* ÉèÖÃºÃÒª·¢ËÍÊý¾Ý°üµÄÇ°Èý¸ö×Ö·ûdata[0]£¬data[1]£¬data[2] */
	/* ¸ù¾ÝsizeBlkµÄ´óÐ¡ÉèÖÃÊý¾ÝÇøÊý¾Ý¸öÊýÊÇÈ¡1024×Ö½Ú»¹ÊÇÈ¡128×Ö½Ú*/
	packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;

	/* Êý¾Ý´óÐ¡½øÒ»²½È·¶¨ */
	size = sizeBlk < packetSize ? sizeBlk :packetSize;
	
	/* Ê××Ö½Ú£ºÈ·¶¨ÊÇ1024×Ö½Ú»¹ÊÇÓÃ128×Ö½Ú */
	if (packetSize == PACKET_1K_SIZE)
	{
		data[0] = STX;
	}
	else
	{
		data[0] = SOH;
	}
	
	/* µÚ2¸ö×Ö½Ú£ºÊý¾ÝÐòºÅ */
	data[1] = pktNo;
	/* µÚ3¸ö×Ö½Ú£ºÊý¾ÝÐòºÅÈ¡·´ */
	data[2] = (~pktNo);
	file_ptr = SourceBuf;

	/* Ìî³äÒª·¢ËÍµÄÔ­Ê¼Êý¾Ý */
	for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
	{
		data[i] = *file_ptr++;
	}
	/* ²»×ãµÄ²¹ EOF (0x1A) »ò 0x00 */
	if ( size  <= packetSize)
	{
		for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
		{
			data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
		}
	}
    sendsize += size;
    printf("SendSize = %d\r\n", sendsize);
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: UpdateCRC16
*	¹¦ÄÜËµÃ÷: ÉÏ´Î¼ÆËãµÄCRC½á¹û crcIn ÔÙ¼ÓÉÏÒ»¸ö×Ö½ÚÊý¾Ý¼ÆËãCRC
*	ÐÎ    ²Î: crcIn ÉÏÒ»´ÎCRC¼ÆËã½á¹û
*             byte  ÐÂÌí¼Ó×Ö½Ú
*	·µ »Ø Öµ: ÎÞ
*********************************************************************************************************
*/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte | 0x100;

  do
  {
	crc <<= 1;
	in <<= 1;
	if(in & 0x100)
		++crc;
	if(crc & 0x10000)
		crc ^= 0x1021;
  }while(!(in & 0x10000));

  return crc & 0xffffu;
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: Cal_CRC16
*	¹¦ÄÜËµÃ÷: ¼ÆËãÒ»´®Êý¾ÝµÄCRC
*	ÐÎ    ²Î: data  Êý¾Ý
*             size  Êý¾Ý³¤¶È
*	·µ »Ø Öµ: CRC¼ÆËã½á¹û
*********************************************************************************************************
*/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
	uint32_t crc = 0;
	const uint8_t* dataEnd = data+size;

	while(data < dataEnd)
		crc = UpdateCRC16(crc, *data++);

	crc = UpdateCRC16(crc, 0);
	crc = UpdateCRC16(crc, 0);

	return crc&0xffffu;
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: CalChecksum
*	¹¦ÄÜËµÃ÷: ¼ÆËãÒ»´®Êý¾Ý×ÜºÍ
*	ÐÎ    ²Î: data  Êý¾Ý
*             size  Êý¾Ý³¤¶È
*	·µ »Ø Öµ: ¼ÆËã½á¹ûµÄºó8Î»
*********************************************************************************************************
*/
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd )
    sum += *data++;

  return (sum & 0xffu);
}

/*
*********************************************************************************************************
*	º¯ Êý Ãû: Ymodem_Transmit
*	¹¦ÄÜËµÃ÷: ·¢ËÍÎÄ¼þ
*	ÐÎ    ²Î: buf  ÎÄ¼þÊý¾Ý
*             sendFileName  ÎÄ¼þÃû
*             sizeFile    ÎÄ¼þ´óÐ¡
*	·µ »Ø Öµ: 0  ÎÄ¼þ·¢ËÍ³É¹¦
*********************************************************************************************************
*/
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
	uint8_t filename[FILE_NAME_LENGTH];
	uint8_t *buf_ptr, tempCheckSum;
	uint16_t tempCRC;
	uint16_t blkNumber;
	uint8_t receivedC[2], CRC16_F = 0, i;
	uint32_t errors, ackReceived, size = 0, pktSize;

	printf("Ymodem_Transmit \n");

	errors = 0;
	ackReceived = 0;
	for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
	{
		filename[i] = sendFileName[i];
	}

	CRC16_F = 1;

	/* ³õÊ¼»¯Òª·¢ËÍµÄµÚÒ»¸öÊý¾Ý°ü */
	Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  	//printf("PACKET_SIZE + PACKET_HEADER = %d \n",PACKET_SIZE + PACKET_HEADER);
	do 
	{
		/* ·¢ËÍÊý¾Ý°ü */
		UART_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* ¸ù¾ÝCRC16_F·¢ËÍCRC»òÕßÇóºÍ½øÐÐÐ£Ñé */
		if (CRC16_F)
		{
			tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
			UART_SendByte(tempCRC >> 8);
			UART_SendByte(tempCRC & 0xFF);
		}
		else
		{
			tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
			UART_SendByte(tempCheckSum);
		}
  
		/* µÈ´ý Ack ºÍ×Ö·û 'C' */
		if (UART_ReceivePacket(&receivedC[0], 2, 10000) == 0)  
		{
			if ((receivedC[0] == ACK)&&(receivedC[1] == CRC16))
			{ 
				/* ½ÓÊÕµ½Ó¦´ð */
				//printf("ackReceived = 1 \n");
				ackReceived = 1;
			}
			else if ((receivedC[0] == CA)&&(receivedC[1] == CA))  //ÖÐÖ¹ÐÅºÅ
			{ 
				/* ½ÓÊÕµ½Ó¦´ð */
				//printf("ackReceived = 1 \n");
				errors =  0x0A;
				break;
			}
		}
		/* Ã»ÓÐµÈµ½ */
		else
		{
		//	printf("errors = %d \n",errors);
			errors++;
		}
	/* ·¢ËÍÊý¾Ý°üºó½ÓÊÕµ½Ó¦´ð»òÕßÃ»ÓÐµÈµ½¾ÍÍÆ³ö */
	}while (!ackReceived && (errors < 0x0A));
  
	/* ³¬¹ý×î´ó´íÎó´ÎÊý¾ÍÍË³ö */
	if (errors >=  0x0A)
	{
		printf("errors = %d return\n",errors);
		return errors;
	}
	
	buf_ptr = buf;
	size = sizeFile;
	blkNumber = 0x01;

	/* ÏÂÃæÊ¹ÓÃµÄÊÇ·¢ËÍ1024×Ö½ÚÊý¾Ý°ü */
	/* Resend packet if NAK  for a count of 10 else end of communication */
	while (size)
	{
		/* ×¼±¸ÏÂÒ»°üÊý¾Ý */
		Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
		ackReceived = 0;
		receivedC[0]= 0;
		errors = 0;
		do
		{
			/* ·¢ËÍÏÂÒ»°üÊý¾Ý */
			if (size >= PACKET_1K_SIZE)
			{
				pktSize = PACKET_1K_SIZE;
			}
			else
			{
				pktSize = PACKET_SIZE;
			}
			
			UART_SendPacket(packet_data, pktSize + PACKET_HEADER);
			
			/* ¸ù¾ÝCRC16_F·¢ËÍCRCÐ£Ñé»òÕßÇóºÍµÄ½á¹û */
			if (CRC16_F)
			{
				tempCRC = Cal_CRC16(&packet_data[3], pktSize);
				UART_SendByte(tempCRC >> 8);
				UART_SendByte(tempCRC & 0xFF);
			}
			else
			{
				tempCheckSum = CalChecksum (&packet_data[3], pktSize);
				UART_SendByte(tempCheckSum);
			}

			/* µÈµ½AckÐÅºÅ */
			if ((UART_ReceiveByte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
			{
				ackReceived = 1; 
				/* ÐÞ¸Äbuf_ptrÎ»ÖÃÒÔ¼°size´óÐ¡£¬×¼±¸·¢ËÍÏÂÒ»°üÊý¾Ý */
				if (size > pktSize)
				{
					buf_ptr += pktSize;  
					size -= pktSize;
					if (blkNumber == ((2*1024*1024)/128))
					{
						return 0xFF; /* ´íÎó */
					}
					else
					{
						blkNumber++;
					}
				}
				else
				{
					buf_ptr += pktSize;
					size = 0;
				}
			}
			else
			{
				errors++;
			}
			
		}while(!ackReceived && (errors < 0x0A));
		
		/* ³¬¹ý10´ÎÃ»ÓÐÊÕµ½Ó¦´ð¾ÍÍË³ö */
		if (errors >=  0x0A)
		{
			return errors;
		} 
	}
	
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;
	do 
	{
		UART_SendByte(EOT);
		
		/* ·¢ËÍEOTÐÅºÅ */
		/* µÈ´ýAckÓ¦´ð */
		if ((UART_ReceiveByte(&receivedC[0], 10000) == 0) )
		{
            if(receivedC[0] == ACK)
            {
               ackReceived = 1;       
            }	 	
		}
		else
		{
			errors++;
		}
		
	}while (!ackReceived && (errors < 0x0A));
    

	if (errors >=  0x0A)
	{
		return errors;
	}

    printf("·¢ËÍ½áÊøÐÅºÅ\r\n");

#if 1
	/* ³õÊ¼»¯×îºóÒ»°üÒª·¢ËÍµÄÊý¾Ý */
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;

	packet_data[0] = SOH;
	packet_data[1] = 0;
	packet_data [2] = 0xFF;

	/* Êý¾Ý°üµÄÊý¾Ý²¿·ÖÈ«²¿³õÊ¼»¯Îª0 */
	for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
	{
		packet_data [i] = 0x00;
	}
  
	do 
	{
		/* ·¢ËÍÊý¾Ý°ü */
		UART_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* ¸ù¾ÝCRC16_F·¢ËÍCRCÐ£Ñé»òÕßÇóºÍµÄ½á¹û */
		tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
		UART_SendByte(tempCRC >> 8);
		UART_SendByte(tempCRC & 0xFF);

		/* µÈ´ý Ack */
		if (UART_ReceiveByte(&receivedC[0], 10000) == 0)  
		{
			if (receivedC[0] == ACK)
			{ 
				/* Êý¾Ý°ü·¢ËÍ³É¹¦ */
				ackReceived = 1;
			}
		}
		else
		{
			errors++;
		}
	}while (!ackReceived && (errors < 0x0A));

        printf("´¦ÀíÍê±Ï\r\n");

	/* ³¬¹ý10´ÎÃ»ÓÐÊÕµ½Ó¦´ð¾ÍÍË³ö */
	if (errors >=  0x0A)
	{
		return errors;
	}  
#endif

	return 0; /* ÎÄ¼þ·¢ËÍ³É¹¦ */
}



char md5_readBuf[64] = {0};

int get_file_md5sum(const char * filename)
{
	FILE * filep;
	char cmd[128] = {"md5sum "};
	
	int ret;

	strcat(cmd,filename);

	filep = popen(cmd,"r");
	if(!filep)
		return -1;
    ret = fread(md5_readBuf,32,1,filep);
 
//    printf("get_file_md5sum = %s\n",md5_readBuf);

    pclose(filep);

    return ret;
}


uint8_t checksum(uint8_t *buf, uint8_t len)
{
	uint8_t sum;
	uint8_t i;

	for(i=0,sum=0; i<len; i++)
		sum += buf[i];

	return sum;
}


void send_update_cmd_tomcu(uint8_t phase);


//·µ»ØÖµÎª0 ±íÊ¾ÒªÉý¼¶£¬ÆäËûÖµ²»Éý¼¶
static int ready_to_update(void)
{
	char data[40] = {0};
	int offset=0;
	uint8_t csum = 0,c,rsum = 0;
	int ret;

	// do{
	// 	ret = UART_ReceiveByte (data, 1);  //¶ÁÈ¡»º´æµÄ×Ö·û
	// }while(ret==0);
	
//	if(data[0] != 0x43)
	{
		send_update_cmd_tomcu(0);
		// do{
		// 	ret = UART_ReceiveByte (data, 1000);  //¶ÁÈ¡»º´æµÄ×Ö·û
		// }while(ret != 0 || *data != 0xa5);
		
		ret = UART_ReceivePacket (data, 35, 2000);
		if(ret == 0)
		{
			rsum = data[34];
			data[34] = 0;
			printf("Receive mcu checksum: %s\n",data+2);

			// for(c=0;c<35;c++)
			// 	printf("%x ",data[c]);
			// printf("\n");

			csum = checksum(data, 34);
			if(csum == rsum)  //Ð£Ñé³É¹¦
			{
				if(memcmp(data+2,md5_readBuf,32)==0) //md5 ÊÇÏàÍ¬µÄ£¬²»Éý¼¶
				{
					printf("md5sum memcmp ret = 0,is the same\n");
					printf("not need update!!!\n");
					return 1;
				}
				else
				{
					printf("md5sum different , readyto update\n");
					send_update_cmd_tomcu(1); //ÐèÒªÉý¼¶
					return 0;
				}	
			}
			else
			{
				printf("checksum error csum = %d,rsum = %d\n",csum,rsum);
				uart_exit();
				return -1;
			}
		}
		else  //串口接收失败！！
		{
			printf("UART_ReceivePacket ret = %d != 0 \n",ret);
			return -1;
		}
	}		

	printf("ready to update!\n");
	return 0;
}


#define ApplicationAddress    0x8006000

/*
*********************************************************************************************************
*	º¯ Êý Ãû: xymodem_send
*	¹¦ÄÜËµÃ÷: ·¢ËÍÎÄ¼þ
*	ÐÎ    ²Î: filename  ÎÄ¼þÃû
*	·µ »Ø Öµ: 0  ÎÄ¼þ·¢ËÍ³É¹¦
*********************************************************************************************************
*/
int xymodem_send(const char *filename)
{
    size_t len;
    int ret, fd;
    FILE *fin, *fout;
    int skip_payload = 0;
    int timeout = 0;
	char data[2] = {0};
    int size = 0;
    int bw = 0;
    int readcount = 0, remain = 0;

	ret = get_file_md5sum(filename);
	if(ret > 0)
	{
		printf("get_file_md5sum = %s,strlen = %lu\n",md5_readBuf,strlen(md5_readBuf));
	}
	else
	{
		printf("error : get_file_md5sum \n");
		return -1;
	}

    fin = fopen(filename, "rb");
    if (fin != NULL)
    {
        /* 文件打开成功*/
        printf("open %s success\r\n",filename);
    }
    else
    {
        printf("open %s error\r\n",filename);
        return -1;
    }

    fseek(fin, 0, SEEK_END);
    size = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    printf("file size = %d\r\n", size);

    char* buf = malloc(size);
    if(buf == NULL)
    {
    	printf("error: malloc %d\n",size);
    	fclose(fin);
    	return -1;
    }


    do
    {
        bw = fread(&buf[readcount], 1, size, fin);
        readcount += bw;
    } while (readcount < size);

    printf("file readcount = %d\r\n", readcount);
    fclose(fin);


    if (((*(uint32_t*)buf) & 0xfFFE0000 ) != 0x20000000)
	{
		printf("image addr 0 != 0x20000000\n");
		return -1;
	}
	else if(((*(uint32_t*)(buf+4)) & 0xfFFff000 ) != ApplicationAddress)
	{
		printf("image  addr 1 != ApplicationAddress\n");
		return -1;
	}


    do{
		ret = UART_ReceiveByte (data, 1);  //ºóÃæ¿ÉÄÜÓÐ1¸ö×Ö·û(²»Ò»¶¨)£¬¶ÁÒ»´ÎÊÔÊÔ£¬Ö»µÈ´ý2ms
	}while(ret==0);

	if(data[0] != 0x43)
	{
		if(ready_to_update())   //·µ»ØÖµ²»Îª0£¬²»ÐèÒªÉý¼¶
			return -1;
	}

	printf("go to update now!!!");	

 //    do{
	// 	ret = UART_ReceiveByte (data, 1);  //ºóÃæ¿ÉÄÜÓÐ1¸ö×Ö·û(²»Ò»¶¨)£¬¶ÁÒ»´ÎÊÔÊÔ£¬Ö»µÈ´ý2ms
	// }while(ret==0);

	do
	{
		printf("wait for mcu ready ...  ... timeout = %d \n",timeout++);
		if(timeout >= 600)   //10·ÖÖÓ¹ýÈ¥ÁË
		{
			//ÔÙ´ÎÖØÆômcu?
			printf("wait for mcu ready timeout,abort now \n");
			//uart_exit();
			return -1;
		}
		ret = UART_ReceiveByte (data,  1000);
		if(ret == 0)
		{
			if(data[0] == 0x43)
			{
			//	printf("recive mcu ready \n");
				break;
			}						
		}

		//printf("wait for mcu ready @@@@\n");
		// else //³¬Ê±Ã»ÓÐÊÕµ½Êý¾ÝÒ²ÊÇ-1
		// {
		// 	printf("ERROR: UART_ReceivePacket\n");
		// 	uart_exit();
		// 	return -1;
		// }
	}while(1);

    Ymodem_Transmit(buf, filename, size);

    return 0;
}

/***************************** °²¸»À³µç×Ó www.armfly.com (END OF FILE) *********************************/

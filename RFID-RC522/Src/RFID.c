#include "MFRC522_STM32.h"
//#include "main.h"
//#include "calculation_crc.h"
#include "RFID.h"

uint8_t RFID_Detect(MFRC522_t *rfID){
/////////////////////////////////////
	uint8_t status;
	status = MFRC522_WakeupA(rfID);
	if (status == STATUS_OK){
		return STATUS_OK;
	}
	else return STATUS_ERROR;
}

uint8_t RFID_Remove(MFRC522_t *rfID){
/////////////////////////////////////
	MFRC522_WriteReg(rfID, PCD_Status2Reg, 0x00);
	MFRC522_AntennaOff(rfID);  // Reset RF
	HAL_Delay(10);  // Allow chip to stabilize
	MFRC522_WriteReg(rfID, PCD_CommandReg, PCD_Idle);

	MFRC522_AntennaOn(rfID);
	HAL_Delay(10);  // Ensure RF is ready

	if (MFRC522_WakeupA(rfID) != STATUS_OK)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		return STATUS_OK; // Card removed, return success
	}

	else return STATUS_ERROR;
}

uint8_t RFID_UID_Read(MFRC522_t *rfID, uint8_t *uid, uint8_t *MENU){
/////////////////////////////////////////////////////
	uint8_t STAT_GLOBAL = MFRC522_Anticoll(rfID, uid);

	if ((STAT_GLOBAL == STATUS_OK) || (STAT_GLOBAL == STATUS_COLL_1))
	{
		USER_LOG("CARD ID:%02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);

		if ((uid[0] == 0x83) && (uid[1] == 0xFB) &&(uid[2] == 0x1B) &&(uid[3] == 0x34)){
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
		}
		else if ((uid[0] == 0x93) && (uid[1] == 0x24) &&(uid[2] == 0x41) &&(uid[3] == 0xCD)){
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
		}


		if (STAT_GLOBAL == STATUS_COLL_1) // Second round
		{
			*MENU = UID_ONLY; //prevent undefined behavior in case of multiple TAGs

			//HALT for the detected TAG
			MFRC522_Select(rfID, uid);
			MFRC522_Halt(rfID);

			//New request for the second TAG
			MFRC522_RequestA(rfID);
			STAT_GLOBAL = MFRC522_Anticoll(rfID, uid);

			if (STAT_GLOBAL == STATUS_OK)
			{
				USER_LOG("CARD ID:%02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);

				if ((uid[0] == 0x83) && (uid[1] == 0xFB) &&(uid[2] == 0x1B) &&(uid[3] == 0x34)){
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
				}
				else if ((uid[0] == 0x93) && (uid[1] == 0x24) &&(uid[2] == 0x41) &&(uid[3] == 0xCD)){
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
				}
			}

			else return STATUS_ERROR;
		}

		return STATUS_OK;
	}

	else if (STAT_GLOBAL == STATUS_COLL_2)
	{
		USER_LOG("More than two cards detected.");
	}

	else return STATUS_ERROR;
}

uint8_t RFID_FullRead(MFRC522_t *rfID, uint8_t *uid){
////////////////////////////////////////////////////
	uint8_t ADDR_Sector;
	uint8_t ADDR_Block;
	uint8_t read_block[18]; // 16 data byte + 2 CRC byte
	uint8_t tag_full[64][16]; //total read_block of one tag in a matrix

	//initialize tag matrix
	for (int i = 0; i<64; i++){
		for (int j = 0; j<16; j++){
			tag_full[i][j] = 0xFF;
		}
	}

	if (MFRC522_Select(rfID, uid) == STATUS_OK){
		USER_LOG("SELECT_SUCCESS");
	}

	else return STATUS_ERROR;

	for (ADDR_Sector = 0; ADDR_Sector < 16; ADDR_Sector++)
	{
		if (MFRC522_Authentication(rfID, uid, ADDR_Sector<<2) == STATUS_OK){
			USER_LOG("AUTH_SUCCESS for sector: %02d", ADDR_Sector);
		}

		else return STATUS_ERROR;

		//USER_LOG("Start full reading...");
		for (ADDR_Block = 0; ADDR_Block < 4; ADDR_Block++)
		{
			if (MFRC522_Read_Block(rfID, ((ADDR_Sector<<2) + ADDR_Block), read_block, sizeof(read_block)) == STATUS_OK)
			{
				//USER_LOG_N("|block-%d| ",ADDR_Block);
				for (int i = 0; i < sizeof(read_block)-2; i++){
					//USER_LOG_N("%02X ",read_block[i]);
					tag_full[(ADDR_Sector<<2) + ADDR_Block][i] = read_block[i];
				}
				//USER_LOG_N("\r\n");
			}

			else return STATUS_ERROR;
		}
	}

	USER_LOG_N("\r\n");
	USER_LOG_N("___Sector: 00_______");
	USER_LOG_N("\r\n");

	for (int i = 0; i<64; i++)
	{
		USER_LOG_N("[%02d]: ",i);

		for (int j = 0; j<16; j++){
			USER_LOG_N("%02X ",tag_full[i][j]);
		}

		USER_LOG_N("\r\n");

		if ((i%4 == 3) && (i != 63))
		{
			USER_LOG_N("\r\n");
			USER_LOG_N("___Sector: %02d_______",(uint8_t)((i/4)+1));
			USER_LOG_N("\r\n");
		}
	}

	USER_LOG_N("\r\n");

	return STATUS_OK;
}

uint8_t RFID_FullErase(MFRC522_t *rfID, uint8_t *uid){
//////////////////////////////////////////////////////
	uint8_t ADDR_Sector;
	uint8_t ADDR_Block;
	uint8_t write_block[16] = {0};

	if (MFRC522_Select(rfID, uid) == STATUS_OK){
		USER_LOG("SELECT_SUCCESS");
	}

	else return STATUS_ERROR;

	USER_LOG("Erasing start...");

	for (ADDR_Sector = 0; ADDR_Sector < 16; ADDR_Sector++)
	{
		if (MFRC522_Authentication(rfID, uid, ADDR_Sector<<2) == STATUS_OK){
			USER_LOG("AUTH_SUCCESS for sector: %02d", ADDR_Sector);
		}

		else return STATUS_ERROR;

		if (ADDR_Sector == 0)
		{
			MFRC522_Write_Block(rfID, 0x01, write_block, sizeof(write_block));
			MFRC522_Write_Block(rfID, 0x02, write_block, sizeof(write_block));
		}

		else
		{
			for (ADDR_Block = 0; ADDR_Block < 3; ADDR_Block++) // keep protected blocks out
			{
				if (((ADDR_Sector<<2) + ADDR_Block) % 4 != 3){ // make sure that protected blocks are in safety
					MFRC522_Write_Block(rfID, ((ADDR_Sector<<2) + ADDR_Block), write_block, sizeof(write_block));
				}
			}
		}
	}

	USER_LOG("Erasing done...");
	return STATUS_OK;
}

uint8_t RFID_ReadBlock(MFRC522_t *rfID, uint8_t *uid, uint8_t MENU_LOCAL){
///////////////////////////////////////////////////////////////////////////
	uint8_t ADDRESS;
	uint8_t read_block[18] = {0};

	if (MFRC522_Select(rfID, uid) == STATUS_OK){
		USER_LOG("SELECT_SUCCESS");
	}

	else return STATUS_ERROR;

	ADDRESS = (uint8_t)((MENU_LOCAL-1)/2);

	if (ADDRESS % 4 != 3) // prohibit protected blocks
	{
		if (MFRC522_Authentication(rfID, uid, ADDRESS) == STATUS_OK){
			USER_LOG("AUTH_SUCCESS for address: %d", ADDRESS);
		}

		else return STATUS_ERROR;

		USER_LOG("Start reading...");

		if (MFRC522_Read_Block(rfID, ADDRESS, read_block, sizeof(read_block)) == STATUS_OK)
		{
			for (int i = 0; i < sizeof(read_block)-2; i++){
				USER_LOG_N("%02X ",read_block[i]);
			}

			USER_LOG_N("\r\n");
		}

		else return STATUS_ERROR;
	}

	else
	{
		USER_LOG("PROTECTED BLOCK!");
	}

	return STATUS_OK;
}

uint8_t RFID_WriteBlock(MFRC522_t *rfID, uint8_t *uid, uint8_t MENU_LOCAL){
///////////////////////////////////////////////////////////////////////////
	uint8_t ADDRESS;
	uint8_t write_block[16] = {0};
	static uint8_t INCR = 0x00;

	if (MFRC522_Select(rfID, uid) == STATUS_OK){
		USER_LOG("SELECT_SUCCESS");
	}

	else return STATUS_ERROR;

	ADDRESS = (uint8_t)((MENU_LOCAL-1)/2);

	if (ADDRESS % 4 != 3) // prohibit protected blocks
	{
		if (MFRC522_Authentication(rfID, uid, ADDRESS) == STATUS_OK){
			USER_LOG("AUTH_SUCCESS for address: %d", ADDRESS);
		}

		else return STATUS_ERROR;

		USER_LOG("Start writing...");

		for (int n = 0; n < sizeof(write_block); n++){
			write_block[n] = INCR++;
		}

		if (MFRC522_Write_Block(rfID, ADDRESS, write_block, sizeof(write_block)) == STATUS_OK)
		{
			for (int i = 0; i < sizeof(write_block); i++){
				USER_LOG_N("%02X ",write_block[i]);
			}

			USER_LOG_N("\r\n");
		}

		else return STATUS_ERROR;
	}

	else
	{
		USER_LOG("PROTECTED BLOCK!");
	}

	return STATUS_OK;
}



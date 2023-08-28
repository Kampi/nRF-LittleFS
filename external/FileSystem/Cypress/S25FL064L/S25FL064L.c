/*****************************************************************************/
/**
* @file S25FL064L.c
*
* Low level driver for the S25FL064L SPI flash memory.
*
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---  --------    -----------------------------------------------
* 1.00  dk   22/04/2022  First release
*
* </pre>
******************************************************************************/

#include "S25FL064L.h"

#define S25FL064L_CMD_RSFDP			0x5A
#define S25FL064L_CMD_SPRP			0xFB
#define S25FL064L_CMD_DPD			0xB9
#define S25FL064L_CMD_RES			0xAB
#define S25FL064L_CMD_RDID			0x9F
#define S25FL064L_CMD_RST			0x99
#define S25FL064L_CMD_RSTEN			0x66
#define S25FL064L_CMD_CE			0x60
#define S25FL064L_CMD_RUID			0x4B
#define S25FL064L_CMD_RDCR1			0x35
#define S25FL064L_CMD_RDCR3			0x33
#define S25FL064L_CMD_CLSR			0x30
#define S25FL064L_CMD_SE			0x21
#define S25FL064L_CMD_RDCR2			0x15
#define S25FL064L_CMD_4READ			0x13
#define S25FL064L_CMD_4PP			0x12
#define S25FL064L_CMD_WREN			0x06
#define S25FL064L_CMD_WRDI			0x04
#define S25FL064L_CMD_RDSR2			0x07
#define S25FL064L_CMD_RDSR1			0x05
#define S25FL064L_CMD_READ			0x03
#define S25FL064L_CMD_PAGE_PROGRAM		0x02

#define S25FL064L_BIT_ADDR_LENGTH		0x00
#define S25FL064L_BIT_BUSY			0x00
#define S25FL064L_BIT_WEL			0x01

/** @brief JEDEC flash parameters.
 */
static flash_params_t Params;

/** @brief		    Send a command code to the Flash memory.
 *  @param p_Device	    Pointer to S25FL064 device structure
 *  @param p_Command	    Pointer to command data
 *  @param CommandLength    Command length
 *  @param p_Data	    Pointer to return data from command
 *  @param Length	    Length of return data
 *  @return		    Error code
 */
static s25fl064_error_t S25FL064L_Command(s25fl064_t* p_Device, uint8_t* p_Command, uint8_t CommandLength, uint8_t* p_Data, uint8_t Length)
{
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    if((p_Device == NULL) || (p_Device->p_RW == NULL) || (p_Device->p_CS == NULL) || ((p_Data == NULL) && (Length > 0)))
    {
	return S25FL064_INVALID_PARAM;
    }

    p_Device->p_CS(true);
    Error = p_Device->p_RW(p_Command, CommandLength, p_Data, Length);
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);
	return Error;
    }
    p_Device->p_CS(false);

    return Error;
}

/** @brief	    Read the manufacturer and the device ID.
 *  @param p_Device Pointer to S25FL064 device structure
 *  @return	    Error code
 */
static s25fl064_error_t S25FL064L_ReadID(s25fl064_t* p_Device)
{
    uint8_t Rx_Buffer[4];
    uint8_t Tx_Buffer = S25FL064L_CMD_RDID;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    p_Device->MID = Rx_Buffer[1];
    p_Device->DID = (((uint16_t)Rx_Buffer[2]) << 0x08) | Rx_Buffer[3];

    return Error;
}

/** @brief	    Read the unique device ID.
 *  @param p_Device Pointer to S25FL064 device structure
 *  @return	    Error code
 */
static s25fl064_error_t S25FL064L_ReadUID(s25fl064_t* p_Device)
{
    uint8_t Rx_Buffer[5];
    s25fl064_error_t Error = S25FL064_NO_ERROR;
    uint8_t Command = S25FL064L_CMD_RUID;

    if((p_Device == NULL) || (p_Device->p_RW == NULL) || (p_Device->p_CS == NULL))
    {
	return S25FL064_INVALID_PARAM;
    }

    p_Device->p_CS(true);

    // Transmit the command and receive the four dummy bytes
    Error = p_Device->p_RW(&Command, sizeof(Command), Rx_Buffer, sizeof(Rx_Buffer));
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);
	return Error;
    }

    // Receive the UID
    Error = p_Device->p_RW(NULL, 0, p_Device->UID, 8);
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);
	return Error;
    }
    p_Device->p_CS(false);

    return Error;
}

/** @brief	    Wait until a pending write process has finished.
 *  @param p_Device Pointer to S25FL064 device structure
 *  @return	    Error code
 */
static s25fl064_error_t S25FL064L_WaitBusy(s25fl064_t* p_Device)
{
    uint8_t Rx_Buffer[2];
    uint8_t Tx_Buffer = S25FL064L_CMD_RDSR1;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    do
    {
	Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
	if(Error != S25FL064_NO_ERROR)
	{
	    return Error;
	}

	if(p_Device->p_Busy)
	{
	    p_Device->p_Busy();
	}
    }while(Rx_Buffer[1] & (0x01 << S25FL064L_BIT_BUSY));

    return Error;
}

/** @brief Read the JEDEC parameter from the flash memory.
 */
static s25fl064_error_t S25FL064L_ReadJEDEC(s25fl064_t* p_Device)
{
    uint8_t Rx_Buffer[5];
    uint8_t Tx_Buffer[] = {S25FL064L_CMD_RSFDP, 0x00, 0x00, 0x00, 0x00};
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    if((p_Device == NULL) || (p_Device->p_RW == NULL) || (p_Device->p_CS == NULL))
    {
	return S25FL064_INVALID_PARAM;
    }

    p_Device->p_CS(true);

    // Transmit the command
    Error = p_Device->p_RW(Tx_Buffer, sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);
	return Error;
    }

    // Receive the data
    Error = p_Device->p_RW(NULL, 0, (uint8_t*)&Params, sizeof(Params));
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);
	return Error;
    }

    p_Device->p_CS(false);

    return Error;
}

s25fl064_error_t S25FL064L_Init(s25fl064_t* p_Device)
{
    uint8_t Rx_Buffer[2];
    uint8_t Tx_Buffer;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    p_Device->isInitialized = false;

    Error = S25FL064L_LeavePowerDown(p_Device);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    // Reset the device for factory defaults
    Error = S25FL064L_Reset(p_Device);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    // Read the device informations
    Error = S25FL064L_ReadID(p_Device) || S25FL064L_ReadUID(p_Device) || S25FL064L_ReadJEDEC(p_Device);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    Tx_Buffer = S25FL064L_CMD_RDCR2;
    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    // Address length status is set. The device will use 4-byte addresses
    if(Rx_Buffer[1] & (0x01 << S25FL064L_BIT_ADDR_LENGTH))
    {
	p_Device->isShortAddress = false;
    }
    // Address length status is not set. The device will use 3-byte addresses
    else
    {
	p_Device->isShortAddress = false;
    }

    p_Device->Impedance = (Rx_Buffer[1] >> 0x05) & 0x03;
    p_Device->isQPI = (Rx_Buffer[1] >> 0x03) & 0x01;
    p_Device->BlockSize = S25FL064L_SECTOR_SIZE;
    p_Device->Blocks = S25FL064L_SECTOR_COUNT;

    p_Device->isInitialized = true;
    p_Device->isPowerDown = false;
    p_Device->isWriteProtect = false;

    return Error;
}

s25fl064_error_t S25FL064L_GetError(s25fl064_t* p_Device, uint8_t* p_Error)
{
    uint8_t Rx_Buffer[2];
    uint8_t Tx_Buffer = S25FL064L_CMD_RDSR2;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    *p_Error = (Rx_Buffer[1] & 0x60) >> 0x05;

    Tx_Buffer = S25FL064L_CMD_CLSR;
    return S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
}

s25fl064_error_t S25FL064L_Reset(s25fl064_t* p_Device)
{
    if((!p_Device) || (p_Device->p_Reset == NULL))
    {
	return S25FL064_INVALID_PARAM;
    }

    p_Device->p_Reset();

    p_Device->isInitialized = false;

    return S25FL064_NO_ERROR;
}

s25fl064_error_t S25FL064L_EnterPowerDown(s25fl064_t* p_Device)
{
    uint8_t Tx_Buffer = S25FL064L_CMD_DPD;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    p_Device->isPowerDown = true;

    return Error;
}

s25fl064_error_t S25FL064L_LeavePowerDown(s25fl064_t* p_Device)
{
    uint8_t Tx_Buffer = S25FL064L_CMD_RES;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    Error = S25FL064L_WaitBusy(p_Device);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    p_Device->isPowerDown = false;

    return Error;
}

s25fl064_error_t S25FL064L_EraseSector(s25fl064_t* p_Device, uint32_t Address)
{
    uint8_t Tx_Buffer[5] = {S25FL064L_CMD_WREN};
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    // Enable write to nonvolatile memory
    Error = S25FL064L_Command(p_Device, &Tx_Buffer[0], sizeof(Tx_Buffer[0]), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    p_Device->p_CS(true);

    Tx_Buffer[0] = S25FL064L_CMD_SE;
    Tx_Buffer[1] = (Address & 0xFF000000) >> 0x18;
    Tx_Buffer[2] = (Address & 0x00FF0000) >> 0x10;
    Tx_Buffer[3] = (Address & 0x0000FF00) >> 0x08;
    Tx_Buffer[4] = (Address & 0x000000FF) >> 0x00;

    // Transmit the set pointer region command and the address
    Error = p_Device->p_RW(Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);

	return Error;
    }

    p_Device->p_CS(false);

    return S25FL064L_WaitBusy(p_Device);
}

s25fl064_error_t S25FL064L_EraseChip(s25fl064_t* p_Device)
{
    uint8_t Tx_Buffer = S25FL064L_CMD_WREN;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    Tx_Buffer = S25FL064L_CMD_CE;
    Error = S25FL064L_Command(p_Device, &Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	return Error;
    }

    return S25FL064L_WaitBusy(p_Device);
}

s25fl064_error_t S25FL064L_Write(s25fl064_t* p_Device, uint32_t Address, const uint8_t* p_Buffer, uint32_t Length)
{
    uint8_t Tx_Buffer[5];
    uint8_t Rx_Buffer[2];
    uint32_t RemainingBytes = Length;
    uint32_t MemoryAddress = Address;
    const uint8_t* p_Buffer_Temp = p_Buffer;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    if((p_Device == NULL) || (p_Buffer == NULL))
    {
	return S25FL064_INVALID_PARAM;
    }

    do
    {
	// Enable write to nonvolatile memory
	Tx_Buffer[0] = S25FL064L_CMD_WREN;
	Error = S25FL064L_Command(p_Device, &Tx_Buffer[0], sizeof(uint8_t), NULL, 0);
	if(Error != S25FL064_NO_ERROR)
	{
	    return Error;
	}

	Tx_Buffer[0] = S25FL064L_CMD_RDSR1;
    	Error = S25FL064L_Command(p_Device, &Tx_Buffer[0], sizeof(Tx_Buffer), Rx_Buffer, sizeof(Rx_Buffer));
    	if(Error != S25FL064_NO_ERROR)
    	{
    	    return Error;
    	}

	// Check if the memory is writeable
	if(!(Rx_Buffer[1] & (0x01 << S25FL064L_BIT_WEL)))
	{
	    return S25FL064_WRITE_PROTECTED;
	}

	Tx_Buffer[0] = S25FL064L_CMD_4PP;
	Tx_Buffer[1] = (MemoryAddress & 0xFF000000) >> 0x18;
	Tx_Buffer[2] = (MemoryAddress & 0x00FF0000) >> 0x10;
	Tx_Buffer[3] = (MemoryAddress & 0x0000FF00) >> 0x08;
	Tx_Buffer[4] = (MemoryAddress & 0x000000FF) >> 0x00;

	p_Device->p_CS(true);

	// Transmit the write command and the address
	Error = p_Device->p_RW(Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
	if(Error != S25FL064_NO_ERROR)
	{
	    p_Device->p_CS(false);

	    return Error;
	}

	// At least one page to transmit
	if(RemainingBytes >= S25FL064L_PAGE_SIZE)
	{
	    // NOTE: nRF52832 specific
	    // The SPI master can transmit up to 255 bytes in a single transaction. So the last byte needs a seperate transaction.
	    // Not an ideal solution, because there is some delay between both transactions (around 32 µs) and it doesn´t make sense 
	    // to add the last byte to the next block of 255 bytes, because the flash memory only supports the writing of one page (256 bytes).
	    Error = p_Device->p_RW(p_Buffer_Temp, 255, NULL, 0) || p_Device->p_RW(p_Buffer_Temp + 255, 1, NULL, 0);
	    if(Error != S25FL064_NO_ERROR)
	    {
		p_Device->p_CS(false);

		return Error;
	    }

	    // Unselect the device and write the page
	    p_Device->p_CS(false);

	    // Wait until the device is ready
	    Error = S25FL064L_WaitBusy(p_Device);
	    if(Error != S25FL064_NO_ERROR)
	    {
		p_Device->p_CS(false);

		return Error;
	    }

	    RemainingBytes -= S25FL064L_PAGE_SIZE;
	    p_Buffer_Temp += S25FL064L_PAGE_SIZE;
	    MemoryAddress += S25FL064L_PAGE_SIZE;
	}
	// Only one page to transmit
	else
	{
	    Error = p_Device->p_RW(p_Buffer, RemainingBytes, NULL, 0);
	    if(Error != S25FL064_NO_ERROR)
	    {
		p_Device->p_CS(false);

		return Error;
	    }

	    // Unselect the device and write the page
	    p_Device->p_CS(false);

	    // Wait until the device is ready
	    Error = S25FL064L_WaitBusy(p_Device);
	    if(Error != S25FL064_NO_ERROR)
	    {
		p_Device->p_CS(false);

		return Error;
	    }

	    RemainingBytes -= RemainingBytes;
	}

	p_Device->p_CS(false);
    } while(RemainingBytes > 0);

    return S25FL064L_WaitBusy(p_Device);
}

s25fl064_error_t S25FL064L_Read(s25fl064_t* p_Device, uint32_t Start, uint8_t* p_Buffer, uint32_t Length)
{
    uint8_t Tx_Buffer[5];
    uint32_t RemainingBytes = Length;
    uint8_t* p_Buffer_Temp = p_Buffer;
    s25fl064_error_t Error = S25FL064_NO_ERROR;

    if((p_Device == NULL) || (p_Buffer == NULL))
    {
	return S25FL064_INVALID_PARAM;
    }

    // Begin the read command by sending the command code and the start address
    Tx_Buffer[0] = S25FL064L_CMD_4READ;
    Tx_Buffer[1] = (Start & 0xFF000000) >> 0x18;
    Tx_Buffer[2] = (Start & 0x00FF0000) >> 0x10;
    Tx_Buffer[3] = (Start & 0x0000FF00) >> 0x08;
    Tx_Buffer[4] = (Start & 0x000000FF) >> 0x00;

    p_Device->p_CS(true);

    Error = p_Device->p_RW(Tx_Buffer, sizeof(Tx_Buffer), NULL, 0);
    if(Error != S25FL064_NO_ERROR)
    {
	p_Device->p_CS(false);

	return Error;
    }

    do
    {
	uint8_t TransmissionLength;

	// NOTE: nRF52832 specific
	// The SPI master can transmit up to 255 bytes in a single transaction. So the last byte needs a seperate transaction.
	// Not an ideal solution, because there is some delay between both transactions (around 32 µs) and it doesn´t make sense 
	// to add the last byte to the next block of 255 bytes, because the flash memory only supports the writing of one page (256 bytes).
	if(RemainingBytes > 255)
	{
	    TransmissionLength = 255;
	}
	else
	{
	    TransmissionLength = RemainingBytes;
	}

	Error = p_Device->p_RW(NULL, 0, p_Buffer_Temp, TransmissionLength);
	if(Error != S25FL064_NO_ERROR)
	{
	    p_Device->p_CS(false);

	    return Error;
	}

	RemainingBytes -= TransmissionLength;
	p_Buffer_Temp += TransmissionLength;
    } while(RemainingBytes > 0);

    p_Device->p_CS(false);

    return Error;
}
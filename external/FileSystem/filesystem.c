/*****************************************************************************/
/**
* @file filesystem.c
*
* Application layer for the LittleFS and the flash memory driver.
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

#define NRF_LOG_MODULE_NAME FileSystem

#include "nrf_gpio.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_rng.h"

#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "lfs.h"
#include "S25FL064L.h"

#include "filesystem.h"

#include "custom_config.h"

/** @brief Buffer size used by the LittleFS.
 */
#define LFS_BUFFER_SIZE				128

NRF_LOG_MODULE_REGISTER();

/** @brief          Flash block read function.
 *  @param p_Config Pointer to LittleFS configuration object
 *  @param Block    Block number
 *  @param Offset   Block offset
 *  @param p_Buffer Pointer to output buffer
 *  @param Size     Number of bytes to read
 *  @return         0 when successful
 *                  -1 when an error occurs
 */
static int Flash_Read(const struct lfs_config* p_Config, lfs_block_t Block, lfs_off_t Offset, void* p_Buffer, lfs_size_t Size);

/** @brief          Flash block write function.
 *  @param p_Config Pointer to LittleFS configuration object
 *  @param Block    Block number
 *  @param Offset   Block offset
 *  @param p_Buffer Pointer to input buffer
 *  @param Size     Number of bytes to write
 *  @return         0 when successful
 *                  -1 when an error occurs
 */
static int Flash_Write(const struct lfs_config* p_Config, lfs_block_t Block, lfs_off_t Offset, const void* p_Buffer, lfs_size_t Size);

/** @brief          Flash block erase function.
 *  @param p_Config Pointer to LittleFS configuration object
 *  @param Block    Block number
 *  @return         0 when successful
 */
static int Flash_Erase(const struct lfs_config* p_Config, lfs_block_t Block);

/** @brief          Flash sync function.
 *  @param p_Config Pointer to LittleFS configuration object
 *  @return         0 when successful
 *                  -1 when an error occurs
 */
static int Flash_Sync(const struct lfs_config* p_Config);

/** @brief  SPI used for the communication with the external flash memory.
 *	    NOTE: TWI and SPI share the same hardware instance!
 */
static const nrf_drv_spi_t SPI_Master		= NRF_DRV_SPI_INSTANCE(0);

/** @brief S25FL064L device instance.
 */
static s25fl064_t Flash;

/** @brief Active watchdog timer ID.
 */
static nrf_drv_wdt_channel_id WDT_Channel_ID;

/** @brief LittleFS file system configuration object.
 */
static struct lfs_config FileSystemConfig =
{
    .context = NULL,

    .read = Flash_Read,
    .prog = Flash_Write,
    .erase = Flash_Erase,
    .sync = Flash_Sync,

    .read_size = LFS_BUFFER_SIZE,
    .prog_size = LFS_BUFFER_SIZE,
    .cache_size = LFS_BUFFER_SIZE,
    .lookahead_size = LFS_BUFFER_SIZE,

    .block_size = S25FL064L_SECTOR_SIZE,
    .block_count = S25FL064L_SECTOR_COUNT,
    .block_cycles = 500,
};

/** @brief
 */
static lfs_t FileSystem;

/** @brief
 */
static lfs_file_t File;

/** @brief Busy handler for the S25FL064L flash memory.
 */
static void Flash_Busy(void)
{
    nrf_drv_wdt_channel_feed(WDT_Channel_ID);
}

/** @brief Reset function for the S25FL064L flash memory.
 */
static void Flash_Reset(void)
{
    nrf_gpio_pin_set(FLASH_SS);
    nrf_delay_ms(100);
    nrf_gpio_pin_clear(FLASH_RESET);
    nrf_delay_ms(1000);
    nrf_gpio_pin_set(FLASH_RESET);
}

/** @brief Chip select function for the S25FL064L flash memory.
 */
static void Flash_CS(bool Select)
{
    if(Select)
    {
	nrf_gpio_pin_clear(FLASH_SS);
    }
    else
    {
	nrf_gpio_pin_set(FLASH_SS);
    }
}

/** @brief		Read/Write function for the flash memory.
 *  @param p_Tx_Buffer	Pointer to transmit buffer
 *  @param Tx_Length	Length of transmit buffer
 *  @param p_Tx_Buffer	Pointer to receive buffer
 *  @param Tx_Length	Length of transmit buffer
 *  @return             #S25FL064_NO_ERROR when successful
 */
static s25fl064_error_t Flash_ReadWrite(const uint8_t* p_Tx_Buffer, uint32_t Tx_Length, uint8_t* p_Rx_Buffer, uint32_t Rx_Length)
{
    if(((p_Tx_Buffer == NULL) && (Tx_Length > 0)) || ((p_Rx_Buffer == NULL) && (Rx_Length > 0)))
    {
	return S25FL064_INVALID_PARAM;
    }

    nrf_drv_spi_transfer(&SPI_Master, p_Tx_Buffer, Tx_Length, p_Rx_Buffer, Rx_Length);

    return S25FL064_NO_ERROR;
}

/** @brief Initialize the SPI module driver.
 */
static void Init_SPI(void)
{
    const nrf_drv_spi_config_t SPI_Config = {
        .ss_pin		= NRF_DRV_SPI_PIN_NOT_USED,
	.miso_pin	= SPI_MISO,
	.mosi_pin	= SPI_MOSI,
	.sck_pin        = SPI_SCLK,
        .irq_priority   = APP_IRQ_PRIORITY_LOWEST,
        .orc            = 0x00,
        .frequency      = NRF_SPI_FREQ_8M,
        .mode           = NRF_DRV_SPI_MODE_0,
        .bit_order      = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
    };

    NRF_LOG_DEBUG(" Initialize SPI...");

    APP_ERROR_CHECK(nrf_drv_spi_init(&SPI_Master, &SPI_Config, NULL, NULL));
}

int Flash_Read(const struct lfs_config* p_Config, lfs_block_t Block, lfs_off_t Offset, void* p_Buffer, lfs_size_t Size)
{
    if(S25FL064L_Read(&Flash, (Block * p_Config->block_size) + Offset, p_Buffer, Size) != S25FL064_NO_ERROR)
    {
	return -1;
    }

    return 0;
}

int Flash_Write(const struct lfs_config* p_Config, lfs_block_t Block, lfs_off_t Offset, const void* p_Buffer, lfs_size_t Size)
{
    if(S25FL064L_Write(&Flash, (Block * p_Config->block_size) + Offset, p_Buffer, Size) != S25FL064_NO_ERROR)
    {
	return -1;
    }

    return 0;
}

int Flash_Erase(const struct lfs_config* p_Config, lfs_block_t Block)
{
    if(S25FL064L_EraseSector(&Flash, Block * p_Config->block_size) != S25FL064_NO_ERROR)
    {
	return -1;
    }

    return 0;
}

int Flash_Sync(const struct lfs_config* p_Config)
{
    UNUSED_PARAMETER(p_Config);

    // NOTE: There is no need to implement this function, because the SPI driver will do all the buffer handling.

    return 0;
}

ret_code_t FileSystem_Init(bool EraseChip, nrf_drv_wdt_channel_id Watchdog)
{
    NRF_LOG_DEBUG(" Initialize Flash memory...");

    WDT_Channel_ID = Watchdog;

    nrf_gpio_cfg_output(FLASH_ENABLE);

    FileSystem_EnableFlash(true);

    nrf_gpio_cfg_output(FLASH_SS);
    nrf_gpio_pin_set(FLASH_SS);

    nrf_gpio_cfg_output(FLASH_RESET);
    nrf_gpio_pin_clear(FLASH_RESET);

    Init_SPI();

    Flash.p_Reset = Flash_Reset;
    Flash.p_CS = Flash_CS;
    Flash.p_RW = Flash_ReadWrite;
    Flash.p_Busy = Flash_Busy;

    if(S25FL064L_Init(&Flash) || (Flash.DID != S25FL064L_DEVICE_ID) || (Flash.isInitialized == false))
    {
	NRF_LOG_ERROR("	Can not initialize Flash memory!");

	return NRF_ERROR_NO_MEM;
    }

    NRF_LOG_DEBUG("	MID: 0x%x", Flash.MID);
    NRF_LOG_DEBUG("	DID: 0x%x", Flash.DID);

    return NRF_SUCCESS;
}

ret_code_t FileSystem_Deinit(void)
{
    if(lfs_file_close(&FileSystem, &File))
    {
	return NRF_ERROR_NO_MEM;
    }

    if(lfs_unmount(&FileSystem))
    {
	return NRF_ERROR_NO_MEM;
    }

    if(S25FL064L_EnterPowerDown(&Flash))
    {
	return NRF_ERROR_NO_MEM;
    }

    FileSystem_EnableFlash(false);

    return NRF_SUCCESS;
}

void FileSystem_EnableFlash(bool Enable)
{
    if(Enable)
    {
        nrf_gpio_pin_clear(FLASH_ENABLE);
    }
    else
    {
        nrf_gpio_pin_set(FLASH_ENABLE);
    }
}

ret_code_t FileSystem_WriteTestFile(void)
{
    int FileError;
    lfs_ssize_t BytesIn;
    lfs_ssize_t BytesOut;
    char Test_Out[] = "Hello, World!";
    char Test_In[sizeof(Test_Out)];

    FileError = lfs_mount(&FileSystem, &FileSystemConfig);
    FileError = lfs_file_open(&FileSystem, &File, "test.txt", LFS_O_RDWR | LFS_O_CREAT);
    BytesIn = lfs_file_write(&FileSystem, &File, Test_Out, sizeof(Test_Out));
    lfs_file_seek(&FileSystem, &File, 0, LFS_SEEK_SET);
    BytesOut = lfs_file_read(&FileSystem, &File, Test_In, sizeof(Test_In));
    FileError = lfs_file_close(&FileSystem, &File);

    if((FileError < 0) || (BytesIn != BytesOut))
    {
        return NRF_ERROR_NO_MEM;
    }

    return NRF_SUCCESS;
}

ret_code_t FileSystem_MemTest(void)
{
    int FileError;
    lfs_ssize_t BytesRead;
    lfs_ssize_t BytesWritten;
    uint32_t Size;
    ret_code_t Error = NRF_SUCCESS;
    uint8_t Buffer_Out[Flash.BlockSize];
    uint8_t Buffer_In[Flash.BlockSize];

    if(nrf_drv_rng_init(NULL))
    {
	return NRF_ERROR_INVALID_STATE;
    }

    NRF_LOG_INFO("Format and mount file system...");
    if(lfs_format(&FileSystem, &FileSystemConfig) || lfs_mount(&FileSystem, &FileSystemConfig))
    {
	NRF_LOG_ERROR(" Can not mount flash memory. Abort!");

	return NRF_ERROR_NO_MEM;
    }
    NRF_LOG_INFO(" Size: %u blocks", lfs_fs_size(&FileSystem));

    for(uint32_t Cycle = 0; Cycle < lfs_fs_size(&FileSystem); Cycle++)
    {
	NRF_LOG_INFO("Cycle %u...", Cycle + 1);

	NRF_LOG_INFO("Generating data for page buffer...");
	nrf_drv_rng_block_rand(Buffer_Out, sizeof(Buffer_Out));

	NRF_LOG_INFO("Write %u bytes...", sizeof(Buffer_Out));
	FileError = lfs_file_open(&FileSystem, &File, "memtest", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
	BytesWritten = lfs_file_write(&FileSystem, &File, Buffer_Out, sizeof(Buffer_Out));
	if((FileError < 0) || (BytesWritten != sizeof(Buffer_Out)))
	{
	    NRF_LOG_ERROR(" Can not write buffer into file!");
	    lfs_file_close(&FileSystem, &File);
	    Error = NRF_ERROR_NO_MEM;

	    goto FileSystem_MemTest_Fail;
	}
	FileError = lfs_file_close(&FileSystem, &File);

	NRF_LOG_INFO("Reading %u bytes...", sizeof(Buffer_In));
	FileError = lfs_file_open(&FileSystem, &File, "memtest", LFS_O_RDONLY) ||
                    lfs_file_seek(&FileSystem, &File, Cycle * sizeof(Buffer_In), LFS_SEEK_SET);
        BytesRead = lfs_file_read(&FileSystem, &File, Buffer_In, sizeof(Buffer_In));
	if((FileError < 0) || (BytesRead != sizeof(Buffer_In)))
	{
	    NRF_LOG_ERROR(" Can not read bytes from file into buffer!");
	    lfs_file_close(&FileSystem, &File);
	    Error = NRF_ERROR_NO_MEM;

	    goto FileSystem_MemTest_Fail;
	}
	FileError = lfs_file_close(&FileSystem, &File);

	for(uint32_t Byte = 0; Byte < sizeof(Buffer_In); Byte++)
	{
	    if(Buffer_In[Byte] != Buffer_Out[Byte])
	    {
		NRF_LOG_ERROR(" Invalid byte! Expected %u - Read %u", Buffer_Out[Byte], Buffer_In[Byte]);
		Error = NRF_ERROR_NO_MEM;

		goto FileSystem_MemTest_Fail;
	    }
	}

	NRF_LOG_FLUSH();
    }

FileSystem_MemTest_Fail:

    NRF_LOG_INFO("Getting file size...");
    Size = lfs_file_size(&FileSystem, &File);
    lfs_file_close(&FileSystem, &File);
    NRF_LOG_INFO("  Size: %u bytes", Size);

    NRF_LOG_INFO("Remove test file...");
    lfs_remove(&FileSystem, "memtest");

    NRF_LOG_INFO("Unmount file system...");
    lfs_unmount(&FileSystem);

    NRF_LOG_FLUSH();

    return Error;
}

ret_code_t FileSystem_RawMemTest(uint32_t* p_FaultSector, uint32_t* p_FaultPage, uint32_t* p_Faultbyte)
{
    uint8_t Page_Out[S25FL064L_PAGE_SIZE];
    uint8_t Page_In[S25FL064L_PAGE_SIZE];

    NRF_LOG_INFO("Erasinjg flash memory...");
    if(S25FL064L_EraseChip(&Flash))
    {
	return NRF_ERROR_NO_MEM;
    }

    if(nrf_drv_rng_init(NULL))
    {
	return NRF_ERROR_INVALID_STATE;
    }

    for(uint32_t Sector = 0; Sector < S25FL064L_SECTOR_COUNT; Sector++)
    {
	uint32_t Pages = S25FL064L_SECTOR_SIZE / S25FL064L_PAGE_SIZE;

        NRF_LOG_INFO("Testing sector %u...", Sector + 1);
	S25FL064L_EraseSector(&Flash, Sector);

        for(uint32_t Page = 0; Page < Pages; Page++)
        {
	    NRF_LOG_INFO("Generating random data for page buffer...");
	    nrf_drv_rng_block_rand(Page_Out, sizeof(Page_Out));

            NRF_LOG_INFO("	Testing page %u / %u...", Page + 1, Pages);
            S25FL064L_Write(&Flash, Page * S25FL064L_PAGE_SIZE, Page_Out, sizeof(Page_Out));
            S25FL064L_Read(&Flash, Page * S25FL064L_PAGE_SIZE, Page_In, sizeof(Page_In));

            for(uint32_t Byte = 0; Byte < S25FL064L_PAGE_SIZE; Byte++)
            {
		if(Page_In[Byte] != Page_Out[Byte])
        	{
		    NRF_LOG_ERROR(" Invalid byte at position %u in page %u! Expected %u - Read %u", Byte, Page, Page_Out[Byte], Page_In[Byte]);

		    if(p_FaultSector)
		    {
			*p_FaultSector = Sector;
		    }

		    if(p_FaultSector)
		    {
			*p_FaultPage = Page;
		    }	    

		    if(p_FaultSector)
		    {
			*p_Faultbyte = Byte;
		    }

		    return NRF_ERROR_NO_MEM;
        	}
            }

	    NRF_LOG_FLUSH();
        }
    }

    return NRF_SUCCESS;
}
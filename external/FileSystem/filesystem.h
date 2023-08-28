/*****************************************************************************/
/**
* @file filesystem.h
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

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

 #include "nrf.h"
 #include "nrf_error.h"
 #include "nrf_drv_wdt.h"

 #include <stdbool.h>

 /** @brief		Initialize the file system.
  *  @param Watchdog	Channel ID of an active watchdog timer to reset the timer during the flash reset
  *  @return		#NRF_SUCCESS when successful
  */
 ret_code_t FileSystem_Init(bool EraseChip, nrf_drv_wdt_channel_id Watchdog);

 /** @brief	Deinitialize the file system and release all ressources.
  *  @return	#NRF_SUCCESS when successful
  */
 ret_code_t FileSystem_Deinit(void);

 /** @brief         Enable / Disable the power supply of the flash memory.
  *  @param Enable  Enable / Disable the flash memory
  */
 void FileSystem_EnableFlash(bool Enable);

 /** @brief	Write and read a test file.
  *  @return	#NRF_SUCCESS when successful
  */
 ret_code_t FileSystem_WriteTestFile(void);

 /** @brief	Run a memory test with the file system API.
  *  @return	#NRF_SUCCESS when successful
  */
 ret_code_t FileSystem_MemTest(void);

 /** @brief		    Run a memory test by using the raw SPI flash API.
  *  @param p_FaultSector   Pointer to faulty sector
  *  @param p_FaultPage	    Pointer to faulty page
  *  @param p_Faultbyte	    Pointer to faulty byte
  *  @return		    #NRF_SUCCESS when successful
  */
 ret_code_t RawMemTest(uint32_t* p_FaultSector, uint32_t* p_FaultPage, uint32_t* p_Faultbyte);

#endif /* FILESYSTEM_H_ */
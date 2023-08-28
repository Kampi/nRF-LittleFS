/*****************************************************************************/
/**
* @file S25FL064L.h
*
* Low level driver for the S25FL064L SPI Flash memory.
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

#ifndef S25FL064L_H_
#define S25FL064L_H_

 #include "S25FL064L_Defs.h"

 /** @brief		Initialize the Flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_Init(s25fl064_t* p_Device);

 /** @brief		Check the device for errors.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @param p_Error	Pointer to error data
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_GetError(s25fl064_t* p_Device, uint8_t* p_Error);

 /** @brief		Reset the Flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_Reset(s25fl064_t* p_Device);

 /** @brief		Enable the power down mode of the Flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_EnterPowerDown(s25fl064_t* p_Device);

 /** @brief		Disables the power down mode of the Flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_LeavePowerDown(s25fl064_t* p_Device);

 /** @brief		Perform a single sector erase.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @param Address	Sector address
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_EraseSector(s25fl064_t* p_Device, uint32_t Address);

 /** @brief		Perform a complete chip erase.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_EraseChip(s25fl064_t* p_Device);

 /** @brief		Write data into the flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @param Address	Page address
  *  @param p_Buffer	Pointer to data buffer
  *  @param Length	Data length
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_Write(s25fl064_t* p_Device, uint32_t Address, const uint8_t* p_Buffer, uint32_t Length);

 /** @brief		Read data from the flash memory.
  *  @param p_Device	Pointer to S25FL064 device structure
  *  @param Address	Start address
  *  @param p_Buffer	Pointer to data buffer
  *  @param Length	Data length
  *  @return		Error code
  */
 s25fl064_error_t S25FL064L_Read(s25fl064_t* p_Device, uint32_t Address, uint8_t* p_Buffer, uint32_t Length);

#endif /* S25FL064L_H_ */
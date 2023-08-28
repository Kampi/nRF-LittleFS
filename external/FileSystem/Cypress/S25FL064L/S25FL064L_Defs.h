/*****************************************************************************/
/**
* @file S25FL064L_Defs.h
*
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---  --------    -----------------------------------------------
* 1.00  dk   03/11/2022  First release
*
* </pre>
******************************************************************************/

#ifndef S25FL064_DEFS_H_
#define S25FL064_DEFS_H_

 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>

 #include "../SFDP.h"

 /** @brief Mask for a programming error from the \ref S25FL064L_GetError function.
  */
 #define S25FL064L_MASK_PROG_ERROR		(0x01 << 0x00)

 /** @brief Mask for a erase error from the \ref S25FL064L_GetError function.
  */
 #define S25FL064L_MASK_ERASE_ERROR		(0x01 << 0x01)

 /** @brief Page write buffer size in bytes.
  */
 #define S25FL064L_PAGE_SIZE			256

 /** @brief Number of sectors.
  */
 #define S25FL064L_SECTOR_COUNT			2048

 /** @brief Size of a sector in bytes.
  */
 #define S25FL064L_SECTOR_SIZE			4096

 /** @brief Device ID of the Flash memory.
  */
 #define S25FL064L_DEVICE_ID			0x6017

 /** @brief Error codes for the S25FL064 driver.
  */
 typedef enum
 {
    S25FL064_NO_ERROR		= 0x00,			    /**< No error. */
    S25FL064_INVALID_PARAM	= 0x01,			    /**< Invalid parameter error. */
    S25FL064_NOT_INITIALIZED	= 0x03,			    /**< Device is not initialized. Please call the
								 \ref S25FL064_Init function. */
    S25FL064_WRITE_PROTECTED	= 0x04,			    /**< Can not write to flash memory. */
 } s25fl064_error_t;

 /** @brief Impedance values used by the S25FL064 driver.
  */
 typedef enum
 {
    S25FL064_IMP_0		= 0x00,			    /**< Impedance selection 0. */
    S25FL064_IMP_1		= 0x01,			    /**< Impedance selection 1. */
    S25FL064_IMP_2		= 0x02,			    /**< Impedance selection 2. */
    S25FL064_IMP_3		= 0x03,			    /**< Impedance selection 3. */
 } s25fl064_imp_t;

 /** @brief Busy function pointer that can be used to reset a watchdog timer etc.
  */
 typedef void (*s25fl06_busy_fptr_t)(void);

 /** @brief Hardware reset function pointer which should be mapped to the platform specific reset function.
  */
 typedef void (*s25fl06_reset_fptr_t)(void);

 /** @brief	    Chip select function pointer which should be mapped to the platform specific chip select function.
  *  @param Select  #true to select the device.
  */
 typedef void (*s25fl06_cs_fptr_t)(bool Select);

 /** @brief		Bus communication function pointer which should be mapped to the platform specific write function.
  *  @param p_Tx_Data	Pointer to transmit data
  *  @param p_Tx_Length	Transmit length
  *  @param p_Rx_Data	Pointer to receive data
  *  @param p_Rx_Length	Receive length
  *  @return		Communication error code
  */
 typedef s25fl064_error_t (*s25fl06_rw_fptr_t)(const uint8_t* p_Tx_Data, uint32_t Tx_Length, uint8_t* p_Rx_Data, uint32_t Rx_Length);
 
 /** @brief S25FL064 status register 1 object structure.
  */
 typedef struct
 {
    uint8_t SRP0:1;					     /**< Status Register Protect 0. */
    uint8_t SEC:1;					     /**< Sector / Block Protect. */
    uint8_t TBPROT:1;					     /**< Top or Bottom Relative Protection. */
    uint8_t BP:3;					     /**< Legacy Block Protection Volatile. */
    uint8_t WEL:1;					     /**< Write Enable Latch. */
    uint8_t WIP:1;					     /**< Write in progress. */
 } __attribute__((packed)) s25fl064_sr1_t;

 /** @brief S25FL064 status register 2 object structure.
  */
 typedef struct
 {
    uint8_t RFU2:1;					     /**< Reserved. */
    uint8_t E_ERR:1;					     /**< Erase Error Occured. */
    uint8_t P_ERR:1;					     /**< Programming Error Occured. */
    uint8_t RFU:3;					     /**< Reserved. */
    uint8_t ES:1;					     /**< Erase Suspend. */
    uint8_t PS:1;					     /**< Program Suspend. */
 } __attribute__((packed)) s25fl064_sr2_t;

 /** @brief S25FL064 device object structure.
  */
 typedef struct
 {
    s25fl06_reset_fptr_t    p_Reset;			    /**< Pointer to S25FL064 reset function. */
    s25fl06_cs_fptr_t	    p_CS;			    /**< Pointer to S25FL064 chip select function. */
    s25fl06_rw_fptr_t	    p_RW;			    /**< Pointer to S25FL064 read/write function. */
    s25fl06_busy_fptr_t	    p_Busy;			    /**< Pointer to S25FL064 busy function. 
								 NOTE: This function can be NULL. */

    bool		    isInitialized;		    /**< Boolean flag to indicate a successful initialization. */
    bool		    isPowerDown;		    /**< Boolean flag to indicate active power down mode. */
    bool		    isWriteProtect;		    /**< Boolean flag to indicate active write protection. */
    bool		    isShortAddress;		    /**< Boolean flag to indicate the 3-byte address mode. */
    bool		    isQPI;			    /**< Boolean flag to indicate QPI mode instead of SPI. */
    uint8_t		    MID;			    /**< Manufacturer ID (0x01 for Cypress). */
    uint16_t		    DID;			    /**< Device ID (0x6017). */
    uint8_t		    UID[8];			    /**< Unique device ID (MSB first). */
    uint32_t		    BlockSize;			    /**< Block size of the device in bytes. */
    uint32_t		    Blocks;			    /**< Number of memory blocks. */

    s25fl064_imp_t	    Impedance;			    /**< Active impedance selection. */
 } s25fl064_t;

#endif /* S25FL064_DEFS_H_ */
/*****************************************************************************/
/**
* @file custom_config.c
*
* Configuration file for the LittleFS example.
*
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date       Changes
* ----- ---- --------   -----------------------------------------------
* 1.00  dk  03/11/2022  First release
*
* </pre>
******************************************************************************/

#ifndef CUSTOM_CONFIG_H_
#define CUSTOM_CONFIG_H_

 #define FLASH_ENABLE					26					/**< Enable signal for the power supply of the flash memory. */
 #define FLASH_RESET                                    28                                      /**< Reset input for the flash memory (active low). */
 #define FLASH_SS                                       03                                      /**< Slave select signal for the flash memory (active low). */

 #define SPI_MOSI                                       30                                      /**< Pin used for the MOSI signal by the SPI module. */
 #define SPI_MISO                                       04                                      /**< Pin used for the MISO signal by the SPI module. */
 #define SPI_SCLK                                       29                                      /**< Pin used for the SCLK signal by the SPI module. */

#endif /* CUSTOM_CONFIG_H_ */
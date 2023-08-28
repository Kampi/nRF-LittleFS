/*****************************************************************************/
/**
* @file SFDP_H_.h
*
* JEDEC SFDP definitions.
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

#ifndef SFDP_H_
#define SFDP_H_

 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>

 /** @brief SFDP magic number ("SFDP").
  */
 #define SFDP_SIGNATURE				0x53454450

 /** @brief SFDP header object.
  */
 typedef struct
 {
    union						    /**< 0x53454450 <=> "SFDP". */
    {
	uint32_t Raw;
	uint8_t ASCII[4];
    } Signature;
    uint8_t Minor;					    /**< */
    uint8_t Major;					    /**< */
    uint8_t NPH;					    /**< */
    uint8_t Unused;					    /**< Unused. */
 } __attribute__((packed)) sfdp_header_t;

  /** @brief SFDP parameter header object.
  */
 typedef struct {
    uint8_t ID_LSB;					    /**< */
    uint8_t Minor;					    /**< */
    uint8_t Major;					    /**< */
    uint8_t Length;					    /**< */
    uint8_t ParameterTablePointer[3];			    /**< */
    uint8_t ID_MSB;					    /**< */
 } __attribute__((packed)) sfdp_parameter_header;

  /** @brief JEDEC flash parameter object.
  */
 typedef struct {
    sfdp_header_t Header;				    /**< */
    sfdp_parameter_header ParamHeader1;			    /**< */
    sfdp_parameter_header ParamHeader2;			    /**< */
 } __attribute__((packed)) flash_params_t;

#endif /* SFDP_H_ */
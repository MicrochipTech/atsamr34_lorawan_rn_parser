/**
 * \file
 *
 * \brief Non volatile memories management
 *
 * Copyright (c) 2013-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#ifndef COMMON_NVM_H_INCLUDED
#define COMMON_NVM_H_INCLUDED

#include "compiler.h"
#include "conf_board.h"
#include "parts.h"
#include "status_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_EXTMEM) && defined(CONF_BOARD_AT45DBX)
#include "at45dbx.h"
#endif

/* ! \name Non volatile memory types */
/* ! @{ */
typedef enum {
	INT_FLASH     /* !< Internal Flash */

#if (XMEGA || UC3 || SAM4S)
	, INT_USERPAGE  /* !< Userpage/User signature */
#endif

#if XMEGA
	, INT_EEPROM    /* !< Internal EEPROM */
#endif

#if defined(USE_EXTMEM) && defined(CONF_BOARD_AT45DBX)
	, AT45DBX        /* !< External AT45DBX dataflash */
#endif
} mem_type_t;
/* ! @} */
#if SAM4L
#       ifndef IFLASH_PAGE_SIZE
#               define IFLASH_PAGE_SIZE FLASH_PAGE_SIZE
#       endif
#       ifndef IFLASH_SIZE
#               define IFLASH_SIZE FLASH_SIZE
#       endif
#else
#       ifndef IFLASH_PAGE_SIZE
#               define IFLASH_PAGE_SIZE IFLASH0_PAGE_SIZE
#       endif

#       ifndef IFLASH_ADDR
#               define IFLASH_ADDR IFLASH0_ADDR
#       endif
#endif

/**
 * \defgroup nvm_group NVM service
 *
 * See \ref common_nvm_quickstart.
 *
 * This is the common API for non volatile memories. Additional features are
 * available
 * in the documentation of the specific modules.
 *
 */

/**
 * \brief Initialize the non volatile memory specified.
 *
 * \param mem Type of non volatile memory to initialize
 */
status_code_t nvm_init(mem_type_t mem);

/**
 * \brief Read single byte of data.
 *
 * \param mem Type of non volatile memory to read
 * \param address Address to read
 * \param data Pointer to where to store the read data
 */
status_code_t nvm_read_char(mem_type_t mem, uint32_t address, uint8_t *data);

/**
 * \brief Write single byte of data.
 *
 * \param mem Type of non volatile memory to write
 * \param address Address to write
 * \param data Data to be written
 */
status_code_t nvm_write_char(mem_type_t mem, uint32_t address, uint8_t data);

/**
 * \brief Read \a len number of bytes from address \a address in non volatile
 * memory \a mem and store it in the buffer \a buffer
 *
 * \param mem Type of non volatile memory to read
 * \param address Address to read
 * \param buffer Pointer to destination buffer
 * \param len Number of bytes to read
 */
status_code_t nvm_read(mem_type_t mem, uint32_t address, void *buffer,
		uint32_t len);

/**
 * \brief Write \a len number of bytes at address \a address in non volatile
 * memory \a mem from the buffer \a buffer
 *
 * \param mem Type of non volatile memory to write
 * \param address Address to write
 * \param buffer Pointer to source buffer
 * \param len Number of bytes to write
 */
status_code_t nvm_write(mem_type_t mem, uint32_t address, void *buffer,
		uint32_t len);

/**
 * \brief Erase a page in the non volatile memory.
 *
 * \param mem Type of non volatile memory to erase
 * \param page_number Page number to erase
 */
status_code_t nvm_page_erase(mem_type_t mem, uint32_t page_number);

/**
 * \brief Get the size of whole non volatile memory specified.
 *
 * \param mem Type of non volatile memory
 * \param size Pointer to where to store the size
 */
status_code_t nvm_get_size(mem_type_t mem, uint32_t *size);

/**
 * \brief Get the size of a page in the non volatile memory specified.
 *
 * \param mem Type of non volatile memory
 * \param size Pointer to where to store the size
 */
status_code_t nvm_get_page_size(mem_type_t mem, uint32_t *size);

/**
 * \brief Get the page number from the byte address \a address.
 *
 * \param mem Type of non volatile memory
 * \param address Byte address of the non volatile memory
 * \param num Pointer to where to store the page number
 */
status_code_t nvm_get_pagenumber(mem_type_t mem, uint32_t address,
		uint32_t *num);

/**
 * \brief Enable security bit which blocks external read and write access
 * to the device.
 *
 */
status_code_t nvm_set_security_bit(void);

/**
 * \page common_nvm_quickstart Quick Start quide for common NVM driver
 *
 * This is the quick start quide for the \ref nvm_group "Common NVM driver",
 * with step-by-step instructions on how to configure and use the driver in a
 * selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section nvm_basic_use_case Basic use case
 * In this basic use case, NVM driver is configured for Internal Flash
 *
 * \section nvm_basic_use_case_setup Setup steps
 *
 * \subsection nvm_basic_use_case_setup_code Example code
 * Add to you application C-file:
 * \code
 *      if(nvm_init(INT_FLASH) == STATUS_OK)
 *        do_something();
 * \endcode
 *
 * \subsection nvm_basic_use_case_setup_flow Workflow
 * -# Ensure that board_init() has configured selected I/Os for TWI function
 * when using external AT45DBX dataflash
 * -# Ensure that \ref conf_nvm.h is present for the driver.
 *   - \note This file is only for the driver and should not be included by the
 * user.
 * -# Call nvm_init \code nvm_init(INT_FLASH); \endcode
 * and optionally check its return code
 *
 * \section nvm_basic_use_case_usage Usage steps
 * \subsection nvm_basic_use_case_usage_code_writing Example code: Writing to
 * non volatile memory
 * Use in the application C-file:
 * \code
 *         uint8_t buffer[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
 *
 *         if(nvm_write(INT_FLASH, test_address, (void *)buffer, sizeof(buffer))
 *==
 *       STATUS_OK)
 *           do_something();
 * \endcode
 *
 * \subsection nvm_basic_use_case_usage_flow Workflow
 * -# Prepare the data you want to send to the non volatile memory
 *   \code uint8_t buffer[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}; \endcode
 * -# Call nvm_write \code nvm_write(INT_FLASH, test_address, (void *)buffer,
 *       sizeof(buffer)) \endcode
 * and optionally check its return value for STATUS_OK.
 *
 * \subsection nvm_basic_use_case_usage_code_reading Example code: Reading from
 * non volatile memory
 * Use in application C-file:
 * \code
 *         uint8_t data_read[8];
 *
 *         if(nvm_read(INT_FLASH, test_address, (void *)data_read,
 *sizeof(data_read))
 *       == STATUS_OK) {
 *           //Check read content
 *           if(data_read[0] == 0xAA)
 *             do_something();
 *         }
 * \endcode
 *
 * \subsection nvm_basic_use_case_usage_flow Workflow
 * -# Prepare a data buffer that will read data from non volatile memory
 *   \code uint8_t data_read[8]; \endcode
 * -# Call nvm_read \code nvm_read(INT_FLASH, test_address, (void *)data_read,
 *       sizeof(data_read)); \endcode
 * and optionally check its return value for STATUS_OK.
 * The data read from the non volatile memory are in data_read.
 *
 * \subsection nvm_basic_use_case_usage_code_erasing Example code: Erasing a
 * page of non volatile memory
 * Use in the application C-file:
 * \code
 *      if(nvm_page_erase(INT_FLASH, test_page) == STATUS_OK)
 *        do_something();
 * \endcode
 *
 * \subsection nvm_basic_use_case_usage_flow Workflow
 * -# Call nvm_page_erase \code nvm_page_erase(INT_FLASH, test_page) \endcode
 * and optionally check its return value for STATUS_OK.
 *
 * \subsection nvm_basic_use_case_usage_code_config Example code: Reading
 * configuration of non volatile memory
 * Use in application C-file:
 * \code
 *         uint8_t mem_size, page_size, page_num;
 *
 *         nvm_get_size(INT_FLASH, &mem_size);
 *         nvm_get_page_size(INT_FLASH, &page_size);
 *         nvm_get_pagenumber(INT_FLASH, test_address, &page_num);
 * \endcode
 *
 * \subsection nvm_basic_use_case_usage_flow Workflow
 * -# Prepare a buffer to store configuration of non volatile memory
 *   \code uint8_t mem_size, page_size, page_num; \endcode
 * -# Call nvm_get_size \code nvm_get_size(INT_FLASH, &mem_size); \endcode
 * and optionally check its return value for STATUS_OK.
 * The memory size of the non volatile memory is in mem_size.
 * -# Call nvm_get_page_size \code nvm_get_page_size(INT_FLASH, &page_size);
 * \endcode
 * and optionally check its return value for STATUS_OK.
 * The page size of the non volatile memory is in page_size.
 * -# Call nvm_get_pagenumber \code nvm_get_page_number(INT_FLASH, test_address,
 *       &page_num); \endcode
 * and optionally check its return value for STATUS_OK.
 * The page number of given address in the non volatile memory is in page_num.
 *
 * \subsection nvm_basic_use_case_usage_code_locking Example code: Enabling
 * security bit
 * Use in the application C-file:
 * \code
 *      if(nvm_set_security_bit() == STATUS_OK)
 *        do_something();
 * \endcode
 *
 * \subsection nvm_basic_use_case_usage_flow Workflow
 * -# Call nvm_set_security_bit \code nvm_set_security_bit() \endcode
 * and optionally check its return value for STATUS_OK.
 */

#ifdef __cplusplus
}
#endif

#endif /* COMMON_NVM_H_INCLUDED */

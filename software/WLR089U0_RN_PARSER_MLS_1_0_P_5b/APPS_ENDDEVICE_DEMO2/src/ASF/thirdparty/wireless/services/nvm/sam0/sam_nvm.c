/**
 * \file
 *
 * \brief Non volatile memories management for SAM devices
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
#include "nvm.h"
#include "common_nvm.h"
#include "conf_board.h"
#include "system_interrupt.h"
#include "string.h"

static status_code_t nvm_sam0_read(mem_type_t mem, uint32_t address,
		uint8_t *const buffer,
		uint32_t len);
static enum status_code nvm_memcpy(
		const uint32_t destination_address,
		uint8_t *const buffer,
		uint16_t length,
		bool erase_flag);

/**
 * \internal Pointer to the NVM MEMORY region start address
 */
#define NVM_MEMORY        ((volatile uint16_t *)FLASH_ADDR)
status_code_t nvm_read(mem_type_t mem, uint32_t address, void *buffer,
		uint32_t len)
{
	status_code_t status = nvm_sam0_read(mem, address, buffer, len);
	return status;//STATUS_OK;
}

status_code_t nvm_sam0_read(mem_type_t mem, uint32_t address,
		uint8_t *const buffer,
		uint32_t len)
{
	switch (mem) {
		

	case INT_FLASH:
    {
		/* Get a pointer to the module hardware instance */
		Nvmctrl *const nvm_module = NVMCTRL;
		/* Check if the module is busy */
		if (!nvm_is_ready()) {
			return STATUS_BUSY;
		}

		/* Clear error flags */
		nvm_module->STATUS.reg = NVMCTRL_STATUS_MASK;

		uint32_t page_address = address / 2;

		/* NVM _must_ be accessed as a series of 16-bit words, perform
		 * manual copy
		 * to ensure alignment */
		for (uint16_t i = 0; i < len; i += 2) {
			/* Fetch next 16-bit chunk from the NVM memory space */
			uint16_t data = NVM_MEMORY[page_address++];

			/* Copy first byte of the 16-bit chunk to the
			 *destination buffer */
			buffer[i] = (data & 0xFF);

			/* If we are not at the end of a read request with an
			 * odd byte count,
			 * store the next byte of data as well */
			if (i < (len - 1)) {
				buffer[i + 1] = (data >> 8);
			}
		}
     }

		break;

	default:
		return ERR_INVALID_ARG;
	}

	return STATUS_OK;
}

enum status_code nvm_memcpy(
		const uint32_t destination_address,
		uint8_t *const buffer,
		uint16_t length,
		bool erase_flag)
{
	enum status_code error_code = STATUS_OK;
	uint8_t row_buffer[NVMCTRL_ROW_PAGES * FLASH_PAGE_SIZE];
	volatile uint8_t *dest_add = (uint8_t *)destination_address;
	const uint8_t *src_buf = buffer;
	uint32_t i;

	/* Calculate the starting row address of the page to update */
	uint32_t row_start_address
		= destination_address &
			~((FLASH_PAGE_SIZE * NVMCTRL_ROW_PAGES) - 1);

	while (length) {
		/* Backup the contents of a row */
		for (i = 0; i < NVMCTRL_ROW_PAGES; i++) {
			do {
				error_code = nvm_read_buffer(
						row_start_address +
						(i * FLASH_PAGE_SIZE),
						(row_buffer +
						(i * FLASH_PAGE_SIZE)),
						FLASH_PAGE_SIZE);
			} while (error_code == STATUS_BUSY);

			if (error_code != STATUS_OK) {
				return error_code;
			}
		}

		/* Update the buffer if necessary */
		for (i = row_start_address;
				i < row_start_address +
				(FLASH_PAGE_SIZE * NVMCTRL_ROW_PAGES); i++) {
			if (length && ((uint8_t *)i == dest_add)) {
				row_buffer[i - row_start_address] = *src_buf++;
				dest_add++;
				length--;
			}
		}

		system_interrupt_enter_critical_section();

		if (erase_flag) {
			/* Erase the row */
			do {
				error_code = nvm_erase_row(row_start_address);
			} while (error_code == STATUS_BUSY);

			if (error_code != STATUS_OK) {
				return error_code;
			}
		}

		/* Write the updated row contents to the erased row */
		for (i = 0; i < NVMCTRL_ROW_PAGES; i++) {
			do {
				error_code = nvm_write_buffer(
						row_start_address +
						(i * FLASH_PAGE_SIZE),
						(row_buffer +
						(i * FLASH_PAGE_SIZE)),
						FLASH_PAGE_SIZE);
			} while (error_code == STATUS_BUSY);

			if (error_code != STATUS_OK) {
				return error_code;
			}
		}

		system_interrupt_leave_critical_section();

		row_start_address += NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE;
	}

	return error_code;
}

status_code_t nvm_write(mem_type_t mem, uint32_t address, void *buffer,
		uint32_t len)
{
	switch (mem) {
	case INT_FLASH:

		if (STATUS_OK != nvm_memcpy(address, buffer, len, true))
		{
			return ERR_INVALID_ARG;
		}
		break;

	default:
		return ERR_INVALID_ARG;
	}

	return STATUS_OK;
}

status_code_t nvm_init(mem_type_t mem)
{
	if (INT_FLASH == mem) {
		struct nvm_config config;
		/* Get the default configuration */
		nvm_get_config_defaults(&config);

		/* Enable automatic page write mode */
		config.manual_page_write = false;

		/* Set wait state to 1 */
		config.wait_states = 2;

		/* Set the NVM configuration */
		nvm_set_config(&config);

		return STATUS_OK;
	}

	return ERR_INVALID_ARG;
}

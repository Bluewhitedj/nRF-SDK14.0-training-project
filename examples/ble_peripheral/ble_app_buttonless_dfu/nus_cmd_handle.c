/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @example examples/ble_peripheral/ble_app_buttonless_dfu
 *
 * @brief Secure DFU Buttonless Service Application main file.
 *
 * This file contains the source code for a sample application using the proprietary
 * Secure DFU Buttonless Service. This is a template application that can be modified
 * to your needs. To extend the functionality of this application, please find
 * locations where the comment "// YOUR_JOB:" is present and read the comments.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nus_cmd_handle.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define NUS_PACK_MAX_DATA_LENGTH 18

#define NUS_CMD_FDS_WR		1
#define NUS_CMD_FDS_RD		2
#define NUS_CMD_FDS_INC		3

#define NUS_CMD_SPI_WR		4
#define NUS_CMD_SPI_RD		5
#define NUS_CMD_SPI_INC		6

#define NUS_CMD_TWI_WR		7
#define NUS_CMD_TWI_RD		8
#define NUS_CMD_TWI_INC		9

extern uint32_t nus_send_data(uint8_t * src, uint16_t len);
/******************************************/
/*****fds macro and function defination ***/
/****************************************/
/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
    "FDS_EVT_INIT",
    "FDS_EVT_WRITE",
    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD",
    "FDS_EVT_DEL_FILE",
    "FDS_EVT_GC",
};

/* Array to map FDS return values to strings. */
char const * fds_err_str[] =
{
    "FDS_SUCCESS",
    "FDS_ERR_OPERATION_TIMEOUT",
    "FDS_ERR_NOT_INITIALIZED",
    "FDS_ERR_UNALIGNED_ADDR",
    "FDS_ERR_INVALID_ARG",
    "FDS_ERR_NULL_ARG",
    "FDS_ERR_NO_OPEN_RECORDS",
    "FDS_ERR_NO_SPACE_IN_FLASH",
    "FDS_ERR_NO_SPACE_IN_QUEUES",
    "FDS_ERR_RECORD_TOO_LARGE",
    "FDS_ERR_NOT_FOUND",
    "FDS_ERR_NO_PAGES",
    "FDS_ERR_USER_LIMIT_REACHED",
    "FDS_ERR_CRC_CHECK_FAILED",
    "FDS_ERR_BUSY",
    "FDS_ERR_INTERNAL",
};

/* Keep track of the progress of a delete_all operation. */
static struct
{
    bool delete_next;   //!< Delete next record.
    bool pending;       //!< Waiting for an fds FDS_EVT_DEL_RECORD event, to delete the next record.
} m_delete_all;

#define FILE_ID         0x0007  /* The ID of the file to write the records into. */
#define RECORD_KEY      0x1115  /* A key for the first record. */

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

static void fds_evt_handler(fds_evt_t const * p_evt)
{
    NRF_LOG_ERROR("Event: %s received (%s)",
                  fds_evt_str[p_evt->id],
                  fds_err_str[p_evt->result]);

    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_initialized = true;
            }
            break;

        case FDS_EVT_WRITE:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
				NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
            }
        } break;

        case FDS_EVT_DEL_RECORD:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
            }
            m_delete_all.pending = false;
        } break;

        default:
            break;
    }
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized)
    {
        (void) sd_app_evt_wait();
    }
	NRF_LOG_INFO("fds ready");
}


//NUS FDS initialize

void nus_fds_init(void)
{
	ret_code_t rc;

	/* Register first to receive an event when initialization is complete. */
    (void) fds_register(fds_evt_handler);

    NRF_LOG_INFO("Initializing fds...");

    rc = fds_init();
    APP_ERROR_CHECK(rc);

    /* Wait for fds to initialize. */
    wait_for_fds_ready();

	fds_stat_t stat = {0};

    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);

	NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
}

uint32_t nus_fds_cmd_handler(uint8_t cmd, uint8_t const *data_src, uint8_t data_length)
{
	ret_code_t rc;
	fds_record_t        record;
	fds_record_desc_t   record_desc;
	fds_find_token_t    ftok;
	fds_flash_record_t  flash_record;
	static uint8_t __ALIGN(sizeof(uint32_t)) wr_data[NUS_PACK_MAX_DATA_LENGTH];
	uint16_t len;

	switch (cmd)
	{
		case NUS_CMD_FDS_WR:
		
			memcpy(wr_data, data_src, data_length);
			// Set up record.
			record.file_id           = FILE_ID;
			record.key               = RECORD_KEY;
			record.data.p_data       = wr_data;
			record.data.length_words = (data_length + 3) / 4;   /* takes into account any eventual remainder of the division. */
			rc = fds_record_write(NULL, &record);
			if (rc != FDS_SUCCESS)
			{
				NRF_LOG_ERROR("NUS FDS write error")
			}
	
			break;
		case NUS_CMD_FDS_RD:
				
			/* It is required to zero the token before first use. */
			memset(&ftok, 0x00, sizeof(fds_find_token_t));
			/* Loop until all records with the given key and file ID have been found. */
			while (fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS);
		    if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
		    {
		        /* Handle error. */
					NRF_LOG_ERROR("NUS FDS read error")
		    }
		    /* Access the record through the flash_record structure. */
			len = flash_record.p_header->length_words * 4;
			rc = nus_send_data((uint8_t *) flash_record.p_data, len);
				
		    /* Close the record when done. */
		    if (fds_record_close(&record_desc) != FDS_SUCCESS)
		    {
		        /* Handle error. */
		    }

			break;

		case NUS_CMD_FDS_INC:
			
			/* It is required to zero the token before first use. */
			memset(&ftok, 0x00, sizeof(fds_find_token_t));
			/* Loop until all records with the given key and file ID have been found. */
			while (fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS);
		    if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
		    {
		        /* Handle error. */
					NRF_LOG_ERROR("NUS FDS read error")
		    }
			if(flash_record.p_header->length_words *4 < NUS_PACK_MAX_DATA_LENGTH)
			{
				memcpy(wr_data, flash_record.p_data, (flash_record.p_header->length_words * 4));
				(*((uint32_t *)(wr_data + (flash_record.p_header->length_words - 1) * 4)))++;
				record.file_id = FILE_ID;
				record.key = RECORD_KEY;
				record.data.p_data = wr_data;
				record.data.length_words = flash_record.p_header->length_words;
				rc = fds_record_update(&record_desc, &record);
				if (rc != FDS_SUCCESS)
				{
					NRF_LOG_ERROR("NUS FDS update error")
				}
			}
			
			break;

		default:
			break;
			
	}
	return rc;
}

uint32_t nus_spi_cmd_handler(uint8_t cmd, uint8_t const *data_src, uint8_t data_length)
{
	ret_code_t rc;
	
	return rc;
}

uint32_t nus_twi_cmd_handler(uint8_t cmd, uint8_t const *data_src, uint8_t data_length)
{
	ret_code_t rc;
	
	return rc;
}



void nus_cmd_handler(uint8_t const *data_src, uint8_t data_length)
{
	uint32_t err_code;

	nus_data_pack_t nus_data_pack;
	nus_data_pack.cmd = data_src[0];
	nus_data_pack.length = data_src[1];
	nus_data_pack.pdata = data_src + 2;

	if(nus_data_pack.length > NUS_PACK_MAX_DATA_LENGTH)
	{
		NRF_LOG_ERROR("NUS Data pack length too big");
		
	}
	else
	{
		switch (nus_data_pack.cmd)
		{
			case NUS_CMD_FDS_WR:
			case NUS_CMD_FDS_RD:
			case NUS_CMD_FDS_INC:
				err_code = nus_fds_cmd_handler(nus_data_pack.cmd, nus_data_pack.pdata, nus_data_pack.length);
				APP_ERROR_CHECK(err_code);
				break;
			case NUS_CMD_SPI_WR:
			case NUS_CMD_SPI_RD:
			case NUS_CMD_SPI_INC:
				err_code = nus_spi_cmd_handler(nus_data_pack.cmd, nus_data_pack.pdata, nus_data_pack.length);
				APP_ERROR_CHECK(err_code);
				break;
			case NUS_CMD_TWI_WR:
			case NUS_CMD_TWI_RD:
			case NUS_CMD_TWI_INC:
				err_code = nus_twi_cmd_handler(nus_data_pack.cmd, nus_data_pack.pdata, nus_data_pack.length);
				APP_ERROR_CHECK(err_code);
				break;
	
			default:
				break;
		}
	}
}



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
#define NUS_PACK_MAX_LENGTH 20


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

#define DJ_DEBUG 0
#if DJ_DEBUG
//dj test
static uint8_t tx_buf[] = {0x07,0x04,0x11,0x22,0x33,0x44};
static uint8_t rx_buf[20];
static uint8_t tx_data_length = sizeof(tx_buf);
static uint8_t rx_data_length = 0;
#endif


#define FILE_ID         0x0008  /* The ID of the file to write the records into. */
#define RECORD_KEY      0x1116  /* A key for the first record. */

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

/*********************************************/
/**************TWI master ************/
/**********************************************/

/* Master Configuration */
#define MASTER_TWI_INST     1       //!< TWI interface used as a master 
#define TWI_SCL_M           27       //!< Master SCL pin.
#define TWI_SDA_M           26       //!< Master SDA pin.
#define NUS_TWI_SLAVE_ADD   0x55

static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(MASTER_TWI_INST);

/**
 * @brief Initialize the master TWI.
 *
 * Function used to initialize the master TWI interface that would communicate with simulated EEPROM.
 *
 * @return NRF_SUCCESS or the reason of failure.
 */
static ret_code_t twi_master_init(void)
{
    ret_code_t ret;
    const nrf_drv_twi_config_t config =
    {
       .scl                = TWI_SCL_M,
       .sda                = TWI_SDA_M,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    ret = nrf_drv_twi_init(&m_twi_master, &config, NULL, NULL);

    if (NRF_SUCCESS == ret)
    {
        nrf_drv_twi_enable(&m_twi_master);
    }

    return ret;
}

void nus_twi_init(void)
{
	ret_code_t err_code;

	NRF_LOG_INFO("TWI init");
	
	/* Initializing TWI master interface for EEPROM */
    err_code = twi_master_init();
    APP_ERROR_CHECK(err_code);
	#if DJ_DEBUG

	while(1)
	{
		nrf_drv_twi_tx(&m_twi_master, NUS_TWI_SLAVE_ADD, tx_buf, tx_data_length, 0);

	}
	#endif
}


/*********************************************/
/**************SPI master ************/
/**********************************************/

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */


void nus_spim_init(void)
{
    NRF_LOG_INFO("SPIM init.");

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
	spi_config.frequency = (nrf_drv_spi_frequency_t)SPI_DEFAULT_FREQUENCY;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));

#if DJ_DEBUG
	//dj test
//	while(1)
	{
		nrf_drv_spi_transfer(&spi, tx_buf, tx_data_length, NULL, 0);
	}
#endif

}
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
	static uint8_t tx_buf[NUS_PACK_MAX_LENGTH];
	uint8_t *p_buf;
	static uint8_t __ALIGN(sizeof(uint32_t)) wr_data[NUS_PACK_MAX_DATA_LENGTH];


	switch (cmd)
	{
		case NUS_CMD_FDS_WR:
		
			p_buf = wr_data;
			for(uint8_t i = 0; i < (data_length + 3)/4; i++)
			{
				
				p_buf[4*i] = data_src[4*i+3];
				p_buf[4*i+1] = data_src[4*i+2];
				p_buf[4*i+2] = data_src[4*i+1];
				p_buf[4*i+3] = data_src[4*i];

			}
			// Set up record.
			record.file_id           = FILE_ID;
			record.key               = RECORD_KEY;
			record.data.p_data       = wr_data;
			record.data.length_words = (data_length + 3) / 4;   /* takes into account any eventual remainder of the division. */
			rc = fds_record_write(NULL, &record);
			if (rc != FDS_SUCCESS)
			{
				NRF_LOG_ERROR("NUS FDS write error");
			}
	
			break;
		case NUS_CMD_FDS_RD:

			/* It is required to zero the token before first use. */
			memset(&ftok, 0x00, sizeof(fds_find_token_t));
			
			if(fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_ERR_NOT_FOUND)
			{
				tx_buf[0] = 0;
				data_length = 1;
				rc = nus_send_data(tx_buf, data_length);
				return rc;
			}
			else
			{	/* Loop until all records with the given key and file ID have been found. */
				while (fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS);
			    if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
			    {
			        /* Handle error. */
						NRF_LOG_ERROR("NUS FDS read error");
			    }
			    /* Access the record through the flash_record structure. */
				p_buf = (uint8_t *) flash_record.p_data;
				for(uint8_t i = 0; i < (data_length + 3)/4; i++)
				{
					
					tx_buf[4*i] = p_buf[4*i+3];
					tx_buf[4*i+1] = p_buf[4*i+2];
					tx_buf[4*i+2] = p_buf[4*i+1];
					tx_buf[4*i+3] = p_buf[4*i];

				}
				
				rc = nus_send_data(tx_buf, data_length);
					
			    /* Close the record when done. */
			    if (fds_record_close(&record_desc) != FDS_SUCCESS)
			    {
			        /* Handle error. */
			    }
			}
			
			break;

		case NUS_CMD_FDS_INC:
			
			/* It is required to zero the token before first use. */
			memset(&ftok, 0x00, sizeof(fds_find_token_t));
			if(fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_ERR_NOT_FOUND)
			{
				tx_buf[0] = 0;
				data_length = 1;
				rc = nus_send_data(tx_buf, data_length);
				return rc;
			}
			else
			{		

				/* Loop until all records with the given key and file ID have been found. */
				while (fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS);
			    if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
			    {
			        /* Handle error. */
						NRF_LOG_ERROR("NUS FDS read error");
			    }
				if(flash_record.p_header->length_words *4 < NUS_PACK_MAX_DATA_LENGTH)
				{
					memcpy(wr_data, flash_record.p_data, (flash_record.p_header->length_words * 4));
					(*((uint32_t *)wr_data))++;
					record.file_id = FILE_ID;
					record.key = RECORD_KEY;
					record.data.p_data = wr_data;
					record.data.length_words = flash_record.p_header->length_words;
					rc = fds_record_update(&record_desc, &record);
					if (rc != FDS_SUCCESS)
					{
						NRF_LOG_ERROR("NUS FDS update error");
					}
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
	static uint8_t tx_buf[NUS_PACK_MAX_LENGTH];
	static uint8_t rx_buf[NUS_PACK_MAX_DATA_LENGTH];
	static uint8_t wr_length;
	uint8_t *p_buf;
	
	memset(rx_buf, 0x00, sizeof(rx_buf));
	memset(tx_buf, 0x00, sizeof(tx_buf));
	tx_buf[0] = cmd;
	tx_buf[1] = data_length;
	
	switch (cmd)
	{
		case NUS_CMD_SPI_WR:
			wr_length = data_length;
			p_buf = tx_buf+2;
			for(uint8_t i = 0; i < (data_length + 3)/4; i++)
			{
				
				p_buf[4*i] = data_src[4*i+3];
				p_buf[4*i+1] = data_src[4*i+2];
				p_buf[4*i+2] = data_src[4*i+1];
				p_buf[4*i+3] = data_src[4*i];

			}

			rc = nrf_drv_spi_transfer(&spi, tx_buf, data_length+2, NULL, 0);
			if(rc != NRF_SUCCESS)
			{
				NRF_LOG_ERROR("NUS_CMD_SPI_WR: SPI write error");
				APP_ERROR_CHECK(rc);
			}
			
			break;
		case NUS_CMD_SPI_RD:
			if(wr_length)
			{
				rc = nrf_drv_spi_transfer(&spi, tx_buf, 2, NULL, 0);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_SPI_RD: SPI write error");
					APP_ERROR_CHECK(rc);
				}
				nrf_delay_ms(10);
				
				rc = nrf_drv_spi_transfer(&spi, NULL, 0, rx_buf, data_length);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_SPI_RD: SPI read error");
					APP_ERROR_CHECK(rc);
				}
				NRF_LOG_HEXDUMP_DEBUG(rx_buf, data_length);
				p_buf = tx_buf;
				for(uint8_t i = 0; i < (data_length + 3)/4; i++)
				{
					
					p_buf[4*i] = rx_buf[4*i+3];
					p_buf[4*i+1] = rx_buf[4*i+2];
					p_buf[4*i+2] = rx_buf[4*i+1];
					p_buf[4*i+3] = rx_buf[4*i];

				}
				
				rc = nus_send_data(tx_buf, data_length);
			}
			else
			{
				tx_buf[0] = 0;
				data_length = 1;
				rc = nus_send_data(tx_buf, data_length);
			}
			break;
		case NUS_CMD_SPI_INC:
			if(wr_length)
			{
				rc = nrf_drv_spi_transfer(&spi, tx_buf, 2, NULL, 0);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_SPI_INC: SPI write error");
					APP_ERROR_CHECK(rc);
				}
				
				nrf_delay_ms(10);
				rc = nrf_drv_spi_transfer(&spi, NULL, 0, rx_buf, wr_length);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_SPI_INC: SPI read error");
					APP_ERROR_CHECK(rc);
				}
				NRF_LOG_HEXDUMP_DEBUG(rx_buf, wr_length);
				p_buf = tx_buf;
				for(uint8_t i = 0; i < (wr_length + 3)/4; i++)
				{
					
					p_buf[4*i] = rx_buf[4*i+3];
					p_buf[4*i+1] = rx_buf[4*i+2];
					p_buf[4*i+2] = rx_buf[4*i+1];
					p_buf[4*i+3] = rx_buf[4*i];

				}
				rc = nus_send_data(tx_buf, wr_length);
			}
			else
			{
				tx_buf[0] = 0;
				data_length = 1;
				rc = nus_send_data(tx_buf, data_length);
			}
			break;
		default:
			break;
	}
	
	return rc;
}

uint32_t nus_twi_cmd_handler(uint8_t cmd, uint8_t const *data_src, uint8_t data_length)
{
	ret_code_t rc;
	
	static uint8_t tx_buf[NUS_PACK_MAX_LENGTH];
	static uint8_t rx_buf[NUS_PACK_MAX_DATA_LENGTH];
	static uint8_t wr_length;
	uint8_t *p_buf;

	tx_buf[0] = cmd;
	tx_buf[1] = data_length;

	memset(rx_buf, 0x00, sizeof(rx_buf));
	memset(tx_buf, 0x00, sizeof(tx_buf));
	
	switch (cmd)
	{
		case NUS_CMD_TWI_WR:
			wr_length = data_length;
			p_buf = tx_buf+2;
			for(uint8_t i = 0; i < (data_length + 3)/4; i++)
			{
				
				p_buf[4*i] = data_src[4*i+3];
				p_buf[4*i+1] = data_src[4*i+2];
				p_buf[4*i+2] = data_src[4*i+1];
				p_buf[4*i+3] = data_src[4*i];

			}
			rc = nrf_drv_twi_tx(&m_twi_master, NUS_TWI_SLAVE_ADD, tx_buf, data_length+2, 0);
			if(rc != NRF_SUCCESS)
			{
				NRF_LOG_ERROR("NUS_CMD_TWI_WR: TWI write error");
				APP_ERROR_CHECK(rc);
			}
			
			break;
		case NUS_CMD_TWI_RD:

			if(wr_length)
			{

				rc = nrf_drv_twi_tx(&m_twi_master, NUS_TWI_SLAVE_ADD, tx_buf, 2, 0);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_TWI_RD: TWI write error");
					APP_ERROR_CHECK(rc);
				}
				nrf_delay_ms(10);
				
				rc = nrf_drv_twi_rx(&m_twi_master, NUS_TWI_SLAVE_ADD, rx_buf, data_length);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_TWI_RD: TWI read error");
					APP_ERROR_CHECK(rc);
				}
				NRF_LOG_HEXDUMP_DEBUG(rx_buf, data_length);
				p_buf = tx_buf;
				for(uint8_t i = 0; i < (data_length + 3)/4; i++)
				{
					
					p_buf[4*i] = rx_buf[4*i+3];
					p_buf[4*i+1] = rx_buf[4*i+2];
					p_buf[4*i+2] = rx_buf[4*i+1];
					p_buf[4*i+3] = rx_buf[4*i];

				}
				rc = nus_send_data(tx_buf, data_length);

			}
			else 
			{
				data_length = 1;
				tx_buf[0] = 0;
				rc = nus_send_data(tx_buf, data_length);

			}
			break;
		case NUS_CMD_TWI_INC:
			if(wr_length)
			{
				rc = nrf_drv_twi_tx(&m_twi_master, NUS_TWI_SLAVE_ADD, tx_buf, 2, 0);
				if(rc != NRF_SUCCESS)
				{
					NRF_LOG_ERROR("NUS_CMD_TWI_INC: TWI write error");
					APP_ERROR_CHECK(rc);
				}

			nrf_delay_ms(10);
			
			rc = nrf_drv_twi_rx(&m_twi_master, NUS_TWI_SLAVE_ADD, rx_buf, data_length);
			if(rc != NRF_SUCCESS)
			{
				NRF_LOG_ERROR("NUS_CMD_TWI_INC: TWI Read error");
				APP_ERROR_CHECK(rc);
			}
			NRF_LOG_HEXDUMP_DEBUG(rx_buf, data_length);
			p_buf = tx_buf;
			for(uint8_t i = 0; i < (data_length + 3)/4; i++)
			{
				
				p_buf[4*i] = rx_buf[4*i+3];
				p_buf[4*i+1] = rx_buf[4*i+2];
				p_buf[4*i+2] = rx_buf[4*i+1];
				p_buf[4*i+3] = rx_buf[4*i];
			
			}
			rc = nus_send_data(tx_buf, data_length);
			}
			else
				{

			}
			break;
		default:
			break;
	}
	
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



/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
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
#include "sdk_config.h"
#include "nrf_drv_spis.h"
#include "nrf_drv_twis.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>

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

#define NUS_TWI_SLAVE_ADD 0x55
#define TWI_SCL_S           27       //!< Slave SCL pin.
#define TWI_SDA_S           26       //!< Slave SDA pin.

struct nus_data_pack_s
{
	uint8_t cmd;
	uint8_t length;
	uint8_t const * pdata;
};

typedef struct nus_data_pack_s nus_data_pack_t;


#define SPIS_INSTANCE 1 /**< SPIS instance index. */
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);/**< SPIS instance. */
#define TWIS_INSTANCE 0 /**< TWIS instance index. */
static const nrf_drv_twis_t m_twis = NRF_DRV_TWIS_INSTANCE(TWIS_INSTANCE);


static uint8_t       m_tx_buf[NUS_PACK_MAX_LENGTH];           /**< TX buffer. */
static uint8_t       m_rx_buf[NUS_PACK_MAX_LENGTH];    /**< RX buffer. */
static uint8_t m_tx_length;        /**< Transfer length. */

static uint8_t    	 m_spi_com_buf[NUS_PACK_MAX_DATA_LENGTH];
static uint8_t 		 m_spi_com_buf_length = 0;
static uint8_t    	 m_twi_com_buf[NUS_PACK_MAX_DATA_LENGTH];
static uint8_t 		 m_twi_com_buf_length = 0;

static volatile bool spis_xfer_done; /**< Flag used to indicate that SPIS instance completed the transfer. */

void nus_cmd_handler(uint8_t data_length)
{
	static bool spi_wr_done = false;
	static bool twi_wr_done = false;

	nus_data_pack_t nus_data_pack;
	nus_data_pack.cmd = m_rx_buf[0];
	nus_data_pack.length = m_rx_buf[1];
	nus_data_pack.pdata = m_rx_buf+2;

	memset(m_tx_buf, 0x00, sizeof(m_tx_buf));
	switch (nus_data_pack.cmd)
	{
		case NUS_CMD_SPI_WR:
			memcpy(m_spi_com_buf, nus_data_pack.pdata, nus_data_pack.length);
			m_spi_com_buf_length = nus_data_pack.length;
			spi_wr_done = true;
						
			break;
		case NUS_CMD_SPI_RD:

			if(spi_wr_done && (nus_data_pack.length <= m_spi_com_buf_length))
			{
				memcpy(m_tx_buf, m_spi_com_buf, nus_data_pack.length);
				m_tx_length = nus_data_pack.length;
				
			}
			else
			{
				m_tx_buf[0] = 0;
				m_tx_length = 1;
			}
			break;
		case NUS_CMD_SPI_INC:
			if(spi_wr_done)
			{
				(*((uint32_t *)m_spi_com_buf))++;
				memcpy(m_tx_buf, m_spi_com_buf, m_spi_com_buf_length);
				m_tx_length = m_spi_com_buf_length;
			}
			else
			{
				m_tx_buf[0] = 0;
				m_tx_length = 1;
			}
			break;
		
		case NUS_CMD_TWI_WR:
			memcpy(m_twi_com_buf, nus_data_pack.pdata, nus_data_pack.length);
			m_twi_com_buf_length = nus_data_pack.length;
			twi_wr_done = true;
						
			break;
		case NUS_CMD_TWI_RD:

			if(twi_wr_done && (nus_data_pack.length <= m_twi_com_buf_length))
			{
				memcpy(m_tx_buf, m_twi_com_buf, nus_data_pack.length);
				m_tx_length = nus_data_pack.length;
				
			}
			else
			{
				m_tx_buf[0] = 0;
				m_tx_length = 1;
			}
			break;
		case NUS_CMD_TWI_INC:
			if(twi_wr_done)
			{
				(*((uint32_t *)m_twi_com_buf))++;
				memcpy(m_tx_buf, m_twi_com_buf, m_twi_com_buf_length);
				m_tx_length = m_twi_com_buf_length;
			}
			else
			{
				m_tx_buf[0] = 0;
				m_tx_length = 1;
			}
			break;
		default:
			break;
	}
	

}

    /**
     * @brief Event processing.
     *
     *
     */
    static void twis_event_handler(nrf_drv_twis_evt_t const * const p_event)
    {
        switch (p_event->type)
        {
        case TWIS_EVT_READ_REQ:
            if (p_event->data.buf_req)
            {
                nrf_drv_twis_tx_prepare(&m_twis, m_tx_buf, m_tx_length);
            }
            break;
        case TWIS_EVT_READ_DONE:
            break;
        case TWIS_EVT_WRITE_REQ:
            if (p_event->data.buf_req)
            {
            	memset(m_rx_buf, 0x00, sizeof(m_rx_buf));
                nrf_drv_twis_rx_prepare(&m_twis, m_rx_buf, sizeof(m_rx_buf));
            }
            break;
        case TWIS_EVT_WRITE_DONE:
			NRF_LOG_HEXDUMP_DEBUG(m_rx_buf, p_event->data.rx_amount);
            nus_cmd_handler(p_event->data.rx_amount);
            break;

        case TWIS_EVT_READ_ERROR:
        case TWIS_EVT_WRITE_ERROR:
        case TWIS_EVT_GENERAL_ERROR:
            break;
        default:
            break;
        }
    }

/** @} */

void nus_twis_init(void)
{
	uint32_t err_code;

	
    const nrf_drv_twis_config_t config =
    {
        .addr               = {NUS_TWI_SLAVE_ADD, 0},
        .scl                = TWI_SCL_S,
        .scl_pull           = NRF_GPIO_PIN_PULLUP,
        .sda                = TWI_SDA_S,
        .sda_pull           = NRF_GPIO_PIN_PULLUP,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH
    };

    /* Init TWIS */
    do
    {
        err_code = nrf_drv_twis_init(&m_twis, &config, twis_event_handler);
		APP_ERROR_CHECK(err_code);
        if (NRF_SUCCESS != err_code)
        {
            break;
        }

        nrf_drv_twis_enable(&m_twis);
    }while (0);
}


/**
 * @brief SPIS user event handler.
 *
 * @param event
 */
void spis_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
        spis_xfer_done = true;
        NRF_LOG_INFO(" Transfer completed");
				NRF_LOG_HEXDUMP_DEBUG(m_rx_buf, event.rx_amount); 
				nus_cmd_handler(event.rx_amount);
    }
}

int main(void)
{
    // Enable the constant latency sub power mode to minimize the time it takes
    // for the SPIS peripheral to become active after the CSN line is asserted
    // (when the CPU is in sleep mode).
    NRF_POWER->TASKS_CONSTLAT = 1;

    bsp_board_leds_init();

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("SPIS example");

    nrf_drv_spis_config_t spis_config = NRF_DRV_SPIS_DEFAULT_CONFIG;
    spis_config.csn_pin               = APP_SPIS_CS_PIN;
    spis_config.miso_pin              = APP_SPIS_MISO_PIN;
    spis_config.mosi_pin              = APP_SPIS_MOSI_PIN;
    spis_config.sck_pin               = APP_SPIS_SCK_PIN;

    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spis_event_handler));
	
	nus_twis_init();
    while (1)
    {
        spis_xfer_done = false;

		memset(m_rx_buf, 0, NUS_PACK_MAX_LENGTH);
		APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis, m_tx_buf, m_tx_length, m_rx_buf, NUS_PACK_MAX_LENGTH));
        while (!spis_xfer_done)
        {
            __WFE();
        }

        NRF_LOG_FLUSH();

        bsp_board_led_invert(BSP_BOARD_LED_0);
    }
}

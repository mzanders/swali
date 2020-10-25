/* 
 * This file is part of Swali VSCP, https://www.github.com/swali_vscp.
 * Copyright (c) 2019 Maarten Zanders.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "swali.h"
#include "swali_config.h"
#include "swali_input.h"
#include "swali_output.h"
#include "systick.h"

#define NUM_CHANNELS (SWALI_NUM_INPUTS + SWALI_NUM_OUTPUTS)

typedef struct
{
#if SWALI_NUM_INPUTS > 0
    swali_input_config_t input[SWALI_NUM_INPUTS];
#endif /* SWALI_NUM_INPUTS > 0 */
#if SWALI_NUM_OUTPUTS > 0
    swali_output_config_t output[SWALI_NUM_OUTPUTS];
#endif /* SWALI_NUM_OUTPUTS > 0 */  
} swali_config_t;

swali_config_t * config;

typedef struct
{
#if SWALI_NUM_INPUTS > 0
    swali_input_data_t input[SWALI_NUM_INPUTS];
#endif /* SWALI_NUM_INPUTS > 0 */
#if SWALI_NUM_OUTPUTS > 0
    swali_output_data_t output[SWALI_NUM_OUTPUTS];
#endif /* SWALI_NUM_OUTPUTS > 0 */
} swali_data_t;

swali_data_t data;

typedef enum
{
    input, output, undefined
} swali_channel_t;

swali_channel_t channel_type(uint8_t swali_channel);
uint8_t type_index(uint8_t swali_channel);

void swali_service_tick(void);

void swali_init(uint8_t *configuration, uint8_t max_config_size)
{
    if (sizeof (swali_config_t) > max_config_size)
    {
        while (1)
        {
        };
    }
    config = (swali_config_t *) configuration;

    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
    {
        switch (channel_type(i))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            swali_input_initialize(i, &config->input[type_index(i)], &data.input[type_index(i)]);
#endif
            break;
        case output:
#if SWALI_NUM_OUTPUTS > 0
            swali_output_initialize(i, &config->output[type_index(i)], &data.output[type_index(i)]);
#endif /* SWALI_NUM_OUTPUTS > 0 */
            break;
        }
    }
    systick_register(swali_service_tick);
}

void swali_process(void)
{
    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
    {
        switch (channel_type(i))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            swali_input_process(&data.input[type_index(i)]);
#endif
            break;
        case output:
#if SWALI_NUM_OUTPUTS > 0
            swali_output_process(&data.output[type_index(i)]);
#endif
            break;
        }
    }
}

void swali_send_event(vscp_event_t * event)
{
    /* send the event out on the VSCP bus */
    vscp_send(event);
    /* send the event back to all channels */
    swali_event_handler(event);
}

void swali_event_handler(vscp_event_t * event)
{
    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
    {
        switch (channel_type(i))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            swali_input_handle_event(&data.input[type_index(i)], event);
#endif
            break;
        case output:
#if SWALI_NUM_OUTPUTS > 0
            swali_output_handle_event(&data.output[type_index(i)], event);
#endif
            break;
        }
    }
}

uint8_t swali_read_reg(uint16_t page, uint8_t reg)
{
    uint8_t rv = 0;
    if (page < NUM_CHANNELS)
    {
        switch (channel_type(page))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            rv = swali_input_read_reg(&data.input[type_index(page)], reg);
#endif
            break;
        case output:
#if SWALI_NUM_OUTPUTS > 0
            rv = swali_output_read_reg(&data.output[type_index(page)], reg);
#endif
            break;
        }
    }
    return rv;
}

void swali_write_reg(uint16_t page, uint8_t reg, uint8_t value)
{
    if (page < NUM_CHANNELS)
    {
        switch (channel_type(page))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            swali_input_write_reg(&data.input[type_index(page)], reg, value);
#endif
            break;
        case output:
#if SWALI_NUM_OUTPUTS > 0
            swali_output_write_reg(&data.output[type_index(page)], reg, value);
#endif
            break;
        }
    }
}

void swali_service_tick(void)
{
    static uint8_t counter = 0;

    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
    {
        switch (channel_type(i))
        {
        case input:
#if SWALI_NUM_INPUTS > 0
            swali_input_service_tick(&data.input[type_index(i)], counter);
#endif
            break;
        case output:
            break;
        }
    }

    counter++;
}

swali_channel_t channel_type(uint8_t swali_channel)
{
#if SWALI_NUM_INPUTS > 0
    if (swali_channel < SWALI_NUM_INPUTS)
    {
        return input;
    }
#endif
#if SWALI_NUM_OUTPUTS > 0
    if (swali_channel < SWALI_NUM_INPUTS + SWALI_NUM_OUTPUTS)
    {
        return output;
    }
#endif
    return undefined;

}

uint8_t type_index(uint8_t swali_channel)
{
    uint8_t rv = 0;
    switch (channel_type(swali_channel))
    {
    case input:
        rv = swali_channel;
        break;
    case output:
        rv = swali_channel - SWALI_NUM_INPUTS;
        break;
    case undefined:
    default:
        rv = 0;
        break;
    }
    return rv;
}

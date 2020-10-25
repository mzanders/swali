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
#include "swali_input.h"
#include "time.h"
#include "discrete.h"
#include "vscp.h"

static void input_debounce(void);
static void send_control_event(swali_input_data_t * data);
static void send_button_event(swali_input_data_t * data, uint8_t state);
static void send_info_event(swali_input_data_t * data, uint8_t state);

static void write_flag(swali_input_data_t * data, uint8_t flag, uint8_t value);
static uint8_t read_flag(swali_input_data_t * data, uint8_t flag);

#define FLAG_ENABLE       0x80
#define FLAG_INVERT       0x20
#define FLAG_TYPE_DIM     0x02 // provision for handling dimmers
#define FLAG_TYPE_TOGGLE  0x01

#define REG_ID0           0x00 // read only
#define REG_ID1           0x01 // read only
#define REG_STATE         0x02 // read only
#define REG_ENABLE        0x03 // R/W
#define REG_ZONE          0x04 // R/W
#define REG_SUBZONE       0x05 // R/W
#define REG_TYPE          0x06 // R/W  0 = pushbutton, 1 = toggle switch
#define REG_INVERT        0x07 // R/W  1 = invert

#define SAMPLE_MODULUS    8

void swali_input_initialize(uint8_t swali_channel, swali_input_config_t * config, swali_input_data_t * data)
{
    data->swali_channel = swali_channel;
    data->config = config;
    data->state = 0;
    data->last_switch_state = 0;
    data->switch_shifter = 0;
}

void swali_input_process(swali_input_data_t * data)
{
    // switch goes high
    if (!data->last_switch_state && (data->switch_shifter == 0xFF))
    {
        // always send button press event
        send_button_event(data, 1);
        if (data->config->flags & FLAG_ENABLE)
        {
            send_control_event(data);
        }
        data->last_switch_state = 1;
    }

    // switch goes low
    if (data->last_switch_state && (data->switch_shifter == 0x00))
    {
        // always send button release event
        send_button_event(data, 0);
        if ((data->config->flags & FLAG_ENABLE) && (data->config->flags & FLAG_TYPE_TOGGLE))
        {
            send_control_event(data);
        }
        data->last_switch_state = 0;
    }
}

// DO NOT send events from this function as it would cause recursion
// >> not supported on PIC18F

void swali_input_handle_event(swali_input_data_t * data, vscp_event_t * event)
{
    // channel not enabled? Return!
    if (!(data->config->flags & FLAG_ENABLE))
        return;

    // inputs/slaves get to process info events
    if ((event->vscp_class == VSCP_CLASS1_INFORMATION) &&
            (event->size == 3) &&
            (data->config->zone == event->data[1]) &&
            (data->config->subzone == event->data[2]))
    {
        if (event->vscp_type == VSCP_TYPE_INFORMATION_ON)
            data->state = 1;

        if (event->vscp_type == VSCP_TYPE_INFORMATION_OFF)
            data->state = 0;
    }
}

void swali_input_write_reg(swali_input_data_t * data, uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case REG_ENABLE:
        write_flag(data, FLAG_ENABLE, value);
        break;

    case REG_ZONE:
        data->config->zone = value;
        break;

    case REG_SUBZONE:
        data->config->subzone = value;
        break;

    case REG_TYPE:
        write_flag(data, FLAG_TYPE_TOGGLE, value);
        break;

    case REG_INVERT:
        write_flag(data, FLAG_INVERT, value);
        break;
    }
}

uint8_t swali_input_read_reg(swali_input_data_t * data, uint8_t reg)
{
    uint8_t value = 0;

    switch (reg)
    {
    case REG_ID0:
        value = 'I';
        break;
    case REG_ID1:
        value = 'N';
        break;
    case REG_STATE:
        value = data->state;
        break;
    case REG_ENABLE:
        value = read_flag(data, FLAG_ENABLE);
        break;
    case REG_ZONE:
        value = data->config->zone;
        break;
    case REG_SUBZONE:
        value = data->config->subzone;
        break;
    case REG_TYPE:
        value = read_flag(data, FLAG_TYPE_TOGGLE);
        break;
    case REG_INVERT:
        value = read_flag(data, FLAG_INVERT);
        break;
    }
    return value;
}

static void send_control_event(swali_input_data_t * data)
{
    vscp_event_t tx_event;

    tx_event.priority = VSCP_PRIORITY_MEDIUM;
    tx_event.vscp_class = VSCP_CLASS1_CONTROL;
    tx_event.size = 3;
    tx_event.data[0] = data->swali_channel; // optional;
    tx_event.data[1] = data->config->zone; // all zones;
    tx_event.data[2] = data->config->subzone; // all subzones;

    // toggle the state
    if (data->state)
        tx_event.vscp_type = VSCP_TYPE_CONTROL_TURNOFF;

    else
        tx_event.vscp_type = VSCP_TYPE_CONTROL_TURNON;

    swali_send_event(&tx_event);
}

static void send_button_event(swali_input_data_t * data, uint8_t state)
{
    vscp_event_t tx_event;
    // send pushbutton event
    tx_event.priority = VSCP_PRIORITY_MEDIUM;
    tx_event.vscp_class = VSCP_CLASS1_INFORMATION;
    tx_event.vscp_type = VSCP_TYPE_INFORMATION_BUTTON;
    tx_event.size = 5;
    tx_event.data[0] = state; // button pressed (1) or released (0);
    tx_event.data[1] = 255; // all zones;
    tx_event.data[2] = 255; // all subzones;
    tx_event.data[3] = 0; //MSB always 0
    tx_event.data[4] = data->swali_channel; //this channel
    swali_send_event(&tx_event);
}

void swali_input_service_tick(swali_input_data_t * data, uint8_t counter)
{
    uint8_t discrete;

    if ((data->swali_channel % SAMPLE_MODULUS) == (counter % SAMPLE_MODULUS))
    {
        // by default, we invert the button logic (pull-up)
        if (data->config->flags & FLAG_INVERT)
            discrete = discrete_read(data->swali_channel);
        else
            discrete = !discrete_read(data->swali_channel);

        // update the shift register
        data->switch_shifter = (data->switch_shifter << 1) | discrete;
    }
}

static void write_flag(swali_input_data_t * data, uint8_t flag, uint8_t value)
{
    if (value == 0)
        data->config->flags &= ~flag;
    else
        data->config->flags |= flag;
}

static uint8_t read_flag(swali_input_data_t * data, uint8_t flag)
{
    if (data->config->flags & flag)
        return 1;
    else
        return 0;
}


/* 
 * This file is part of Swali VSCP, https://www.github.com/swali_vscp.
 * Copyright (c) 2020 Maarten Zanders.
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
#include "vscp4hass.h"

static void input_debounce(void);
static void send_control_event(swali_input_data_t * data);
static void send_button_event(swali_input_data_t * data, uint8_t state);
static void send_info_event(swali_input_data_t * data, uint8_t state);

static void write_flag(swali_input_data_t * data, uint8_t flag, uint8_t value);
static uint8_t read_flag(swali_input_data_t * data, uint8_t flag);
static uint8_t reg_range(uint8_t reg);

#define FLAG_ENABLE       0x80
#define FLAG_INVERT       0x20
#define FLAG_TYPE_DIM     0x02 // provision for handling dimmers
#define FLAG_TYPE_TOGGLE  0x01

/* Register map, following VSCP4HASS binary sensor specification
 *   No names are used (return all 0's).
 */
#define REG_ID0           0x00 // read only
#define REG_ID1           0x01 // read only
#define REG_ENABLE        0x02 // R/W
#define REG_STATE         0x03 // read only
#define REG_CLASS_ID      0x04 // R/W
#define REG_NAME          0x10 // R/W max 16chars
#define REG_ZONE          0x20 // R/W
#define REG_SUBZONE       0x21 // R/W
#define REG_TYPE          0x22 // R/W  0 = pushbutton, 1 = toggle switch
#define REG_INVERT        0x23 // R/W  1 = invert

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
        if ((data->config->flags & FLAG_ENABLE) && 
                (data->config->zone != 0xFF))
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
        if ((data->config->flags & FLAG_ENABLE) && 
                (data->config->flags & FLAG_TYPE_TOGGLE) && 
                (data->config->zone != 0xFF))
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
    switch (reg_range(reg))
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
    
    case REG_CLASS_ID:
        if(value <= VSCP4HASS_BS_MAX_CLASS_ID)
            data->config->class_id = value;
        break;
        
    case REG_NAME:
        if (((reg - REG_NAME) < SWALI_NAME_LENGTH) && ((reg - REG_NAME) < 16))
            data->config->name[reg - REG_NAME] = value;
            
    }
}

uint8_t swali_input_read_reg(swali_input_data_t * data, uint8_t reg)
{
    uint8_t value = 0;

    switch (reg_range(reg))
    {
    case REG_ID0:
        value = 'B';
        break;
    case REG_ID1:
        value = 'S';
        break;
    case REG_ENABLE:
        value = read_flag(data, FLAG_ENABLE);
        break;
    case REG_STATE:
        value = data->last_switch_state; // this is the state of the input!
        break;
    case REG_CLASS_ID:
        value = data->config->class_id;
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
    case REG_NAME:
        if (((reg - REG_NAME) < SWALI_NAME_LENGTH) && ((reg - REG_NAME) < 16))
            value = data->config->name[reg - REG_NAME];
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
    tx_event.data[0] = data->swali_channel;
    tx_event.data[1] = data->config->zone;
    tx_event.data[2] = data->config->subzone;

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
    uint8_t class_id = data->config->class_id;
    
    // send event for the configured class ID
    if (class_id > VSCP4HASS_BS_MAX_CLASS_ID)
        return;
    
// temporary only send simple ON/OFF events
    tx_event.vscp_class = vscp4hass_bs_class_map[0].vscp_class;
    if(state)
        tx_event.vscp_type = vscp4hass_bs_class_map[0].vscp_event_on;
    else
        tx_event.vscp_type = vscp4hass_bs_class_map[0].vscp_event_off;
    
    tx_event.priority = VSCP_PRIORITY_MEDIUM;
    
    tx_event.size = 3;
    tx_event.data[0] = data->swali_channel; // this channel
    tx_event.data[1] = 255; // all zones/subzones, this ensures that other 
    tx_event.data[2] = 255; // inputs don't process these as light state updates
    
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

static uint8_t reg_range(uint8_t reg)
{
    uint8_t value = reg;

    if ((reg >= REG_NAME) &&
            (reg < REG_NAME + 16))
        value = REG_NAME;

    return value;
}

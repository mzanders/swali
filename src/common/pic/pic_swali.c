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

#include <xc.h>
#include <string.h>

#include "configuration.h"
#include "discrete.h"
#include "time.h"
#include "pic_swali.h"
#include "led.h"
#include "swali.h"
#include "swali_config.h"

const uint8_t vscp_node_mdf[32] = "use local";
const uint8_t vscp_std_id[8] = "SWALI";

// CONFIGURATION DATA IN EEPROM
// the entire blob of config data
char config_data[256];
const uint16_t config_data_size = sizeof(config_data);


// pointer to swali struct, located at an offset inside config_data
uint8_t *config_swali = (uint8_t*)&(config_data[CONFIG_SWALI]);

void initialize_config_data(void)
{
    config_init((void*) config_data, sizeof (config_data));
    if (config_data[CONFIG_BOOT] != 0xAA)
    {
        // invalid data? clear it out!
        for (int i = 0; i < 255; i++)
        {
            config_data[i] = 0;
        }
        config_data[CONFIG_NICKNAME] = 0xFF;
        config_data[CONFIG_BOOT] = 0xAA;
        config_wait_written(); //takes about 50ms
    }
}

uint8_t process_button(void)
{
    static uint16_t push_start;
    static uint8_t last_state = 1;
    static uint8_t push_ignore = 0;
    uint8_t current_state;
    uint8_t rv = 0;

    current_state = discrete_read(PUSHBUTTON_ID);

    if (!push_ignore)
    {
        if ((current_state == 0) && (last_state == 1))
        {
            push_start = time_get_ms();
        }
        if ((current_state == 0) && (time_get_ms() - push_start) > 2000)
        {
            rv = 1;
            push_ignore = 1;
        }
    }
    else
    {
        if (current_state)
            push_ignore = 0;
    }
    last_state = current_state;
    return rv;
}

void vscp_message_handler(vscp_message_t * message)
{
    switch (message->type)
    {
    case VSCP_MSG_ENTER_BOOT:
        config_data[CONFIG_BOOT] = 0xFF;
        config_wait_written();
        RESET();
        break;

    case VSCP_SET | VSCP_MSG_STATE:
        // drive the LED based on the VSCP state
        switch (message->value[1])
        {
        case VSCP_STATE_STARTUP:
            led_set_state(off);
            break;
        case VSCP_STATE_INIT:
            led_set_state(blink_slow);
            break;
        case VSCP_STATE_ACTIVE:
            led_set_state(on);
            break;
        case VSCP_STATE_ERROR:
            led_set_state(blink_fast);
            break;
        }
        break;

    case VSCP_GET | VSCP_MSG_NICKNAME:
        message->value[1] = config_data[CONFIG_NICKNAME];
        message->length = 2;
        break;

    case VSCP_SET | VSCP_MSG_NICKNAME:
        config_data[CONFIG_NICKNAME] = message->value[1];
        break;

    case VSCP_GET | VSCP_MSG_REGVALUE:
        message->value[3] = swali_read_reg((message->value[1] << 8) | message->value[2], message->value[0]);
        message->length = 4;
        break;

    case VSCP_SET | VSCP_MSG_REGVALUE:
        swali_write_reg((message->value[1] << 8) | message->value[2], message->value[0], message->value[3]);    
        break;
                    
    case VSCP_GET | VSCP_MSG_PAGES_USED:
        message->value[1] = SWALI_NUM_INPUTS + SWALI_NUM_OUTPUTS;
        message->length = 2;
        break;

    case VSCP_GET | VSCP_MSG_GUID:
        if (message->value[0] < 16)
        {
            message->length = 2;
            if (message->value[0] >= 12)
                message->value[1] = config_data[CONFIG_GUID + message->value[0] - 12];
            else
                message->value[1] = 0;
        }
        break;

        // Not really meant to be setting the GUID, but it's convenient so what the heck :-)
    case VSCP_SET | VSCP_MSG_GUID:
        if (message->value[0] < 16)
        {
            if (message->value[0] >= 12)
                config_data[CONFIG_GUID + message->value[0] - 12] = message->value[1];
        }
        break;

        // case VSCP_MSG_GETALARMSTATUS:
        // break;

    case VSCP_GET | VSCP_MSG_USERID:
        if (message->value[0] < 5)
        {
            message->value[1] = config_data[CONFIG_UID + message->value[0]];
            message->length = 2;
        }
        break;

    case VSCP_SET | VSCP_MSG_USERID:
        if (message->value[0] < 5)
            config_data[CONFIG_UID + message->value[0]] = message->value[1];
        break;

    case VSCP_GET | VSCP_MSG_MDF:
        if (message->value[0] < 32)
        {
            message->length = 2;
            if (message->value[0] < strlen(vscp_node_mdf))
                message->value[1] = vscp_node_mdf[message->value[0]];
            else
                message->value[1] = 0;
        }
        break;

    case VSCP_GET | VSCP_MSG_STD_DEVICE:
        if (message->value[0] < 8)
        {
            message->length = 2;
            message->value[1] = vscp_std_id[message->value[0]];
        }
        break;
    }
    return;
}


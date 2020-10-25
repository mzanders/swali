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

#ifndef _SWALI_OUTPUT_H_
#define	_SWALI_OUTPUT_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include "swali_config.h"
#include "vscp.h" 

    typedef struct {
        uint8_t flags;
        uint8_t zone;
        uint8_t subzone;
        uint8_t on_time_hrs;
        uint8_t on_time_mins;
        char name [SWALI_NAME_LENGTH];
    } swali_output_config_t;

    typedef struct {
        uint16_t last_time_ms;
        uint8_t swali_channel;
        uint8_t state;
        uint8_t last_state;
        uint8_t on_time_hrs;
        uint8_t on_time_mins;
        swali_output_config_t * config;
    } swali_output_data_t;

    void swali_output_initialize(uint8_t swali_channel, swali_output_config_t * config, swali_output_data_t * data);
    void swali_output_process(swali_output_data_t * data);
    void swali_output_handle_event(swali_output_data_t * data, vscp_event_t * event);
    void swali_output_write_reg(swali_output_data_t * data, uint8_t reg, uint8_t value);
    uint8_t swali_output_read_reg(swali_output_data_t * data, uint8_t reg);


#ifdef	__cplusplus
}
#endif

#endif	/* _SWALI_OUTPUT_H_ */


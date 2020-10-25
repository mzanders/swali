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

#ifndef _SWALI_H_
#define	_SWALI_H_

#ifdef	__cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "vscp.h"
    
void swali_init(uint8_t *configuration, uint8_t max_config_size);

void swali_process(void);
// processes can use this function to send event (which also gets them back 
// to other channels)
void swali_send_event(vscp_event_t *event);

// incoming events (and internally generated events) go here
void swali_event_handler(vscp_event_t * event);

// reading and writing to VSCP registers
uint8_t swali_read_reg(uint16_t page, uint8_t reg);
void swali_write_reg(uint16_t page, uint8_t reg, uint8_t value);

#ifdef	__cplusplus
}
#endif

#endif	/* _SWALI_H_ */


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

#ifndef _PIC_SWALI_H_
#define	_PIC_SWALI_H_

#ifdef	__cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "vscp.h"
        
extern uint8_t *config_swali;
extern const uint16_t config_data_size;

// special entries in config data
#define CONFIG_BOOT     0x00   // 0xFF = enter boot, 0xAA = valid data
#define CONFIG_NICKNAME 0x01   // 0xFF is invalid
#define CONFIG_GUID     0x02   // 4 bytes, other bytes of GUID set to 0
#define CONFIG_UID      0x06   // 5 bytes
#define CONFIG_SWALI    0x10   // start of swali config struct, max 240bytes


void vscp_message_handler(vscp_message_t * message);
void initialize_config_data(void);
uint8_t process_button(void);
    
#ifdef	__cplusplus
}
#endif 
    
#endif /* _PIC_SWALI_H_ */
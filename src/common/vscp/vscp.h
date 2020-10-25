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

#ifndef _VSCP_H_
#define	_VSCP_H_

#include <stdint.h>

#include "vscp_type.h"
#include "vscp_class.h"
#ifdef	__cplusplus
extern "C" {
#endif

#define VSCP_FALSE                   0x00
#define VSCP_TRUE                    0x01
    
#define VSCP_SET                     0x00
#define VSCP_GET                     0x80
                        
#define VSCP_MSG_STATE               0x00    // L1, V=VSCP_STATE_*
#define VSCP_MSG_NICKNAME            0x01    // L0, reply L2
#define VSCP_MSG_REGVALUE            0x02
#define VSCP_MSG_ENTER_BOOT          0x03
#define VSCP_MSG_BOOT_ALG            0x04
#define VSCP_MSG_GUID                0x05
#define VSCP_MSG_MDF                 0x06
#define VSCP_MSG_DMINFO              0x07
#define VSCP_MSG_ALARMSTATUS         0x08
#define VSCP_MSG_USERID              0x09
#define VSCP_MSG_MFGID               0x0A
#define VSCP_MSG_FWVERSION           0x0B
#define VSCP_MSG_STD_DEVICE          0x0C // FAMILY+SUBFAMILY
#define VSCP_MSG_RESET_CONFIG        0x0D
#define VSCP_MSG_PAGES_USED          0x0E
    
    
    // Value for VSCP_MSG_SETSTATE 
#define VSCP_STATE_STARTUP              0x00	// Cold/warm reset
#define VSCP_STATE_INIT                 0x01	// Assigning nickname
#define VSCP_STATE_ACTIVE               0x02	// The normal state
#define VSCP_STATE_ERROR                0x03	// error state. Big problems.

    // Values for priority
#define VSCP_PRIORITY7                  0x00
#define VSCP_PRIORITY_HIGH              0x00
#define VSCP_PRIORITY6                  0x01
#define VSCP_PRIORITY5                  0x02
#define VSCP_PRIORITY4                  0x03
#define VSCP_PRIORITY_MEDIUM            0x03
#define VSCP_PRIORITY_NORMAL            0x03
#define VSCP_PRIORITY3                  0x04
#define VSCP_PRIORITY2                  0x05
#define VSCP_PRIORITY1                  0x06
#define VSCP_PRIORITY0                  0x07
#define VSCP_PRIORITY_LOW               0x07

    /* Node nickname defines */
#define VSCP_NICKNAME_FREE              0xFF
#define VSCP_NICKNAME_MASTER            0x00


#define MAX_MSG_DATA_LENGTH 8

    typedef struct {
        uint8_t type; // identification of the message
        uint8_t length; // number of bytes used in the next field
        uint8_t value[MAX_MSG_DATA_LENGTH]; // Values to be passed back & forth, depending on type
    } vscp_message_t;

    typedef struct {
        uint8_t priority;    // Priority for the message 0-7
        uint16_t vscp_class; // VSCP class
        uint8_t vscp_type;   // VSCP type
        uint8_t nickname;    // VSCP nickname (filled in by VSCP when sending)
        uint8_t size;        // number of data bytes used
        uint8_t data[8];     // data bytes
    } vscp_event_t;

    void vscp_init(void message_callback(vscp_message_t * message),
            void event_callback(vscp_event_t * event)); // if set to 1, start in the init state!

    void vscp_send(vscp_event_t * event);

    void vscp_process(uint8_t init);


#ifdef	__cplusplus
}
#endif

#endif	/* _VSCP_H_ */


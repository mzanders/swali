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

#ifndef _VSCP_REGISTERS_H_
#define	_VSCP_REGISTERS_H_

#ifdef	__cplusplus
extern "C" {
#endif
    
// ******************************************************************************
//  			VSCP Register - Logical positions
// ******************************************************************************

#define VSCP_REG_ALARMSTATUS                0x80
#define VSCP_REG_VSCP_MAJOR_VERSION         0x81
#define VSCP_REG_VSCP_MINOR_VERSION         0x82

#define VSCP_REG_NODE_ERROR_COUNTER         0x83

#define VSCP_REG_USERID0                    0x84
#define VSCP_REG_USERID1                    0x85
#define VSCP_REG_USERID2                    0x86
#define VSCP_REG_USERID3                    0x87
#define VSCP_REG_USERID4                    0x88

#define VSCP_REG_MANUFACTUR_ID0             0x89
#define VSCP_REG_MANUFACTUR_ID1             0x8A
#define VSCP_REG_MANUFACTUR_ID2             0x8B
#define VSCP_REG_MANUFACTUR_ID3             0x8C

#define VSCP_REG_MANUFACTUR_SUBID0          0x8D
#define VSCP_REG_MANUFACTUR_SUBID1          0x8E
#define VSCP_REG_MANUFACTUR_SUBID2          0x8F
#define VSCP_REG_MANUFACTUR_SUBID3          0x90

#define VSCP_REG_NICKNAME_ID                0x91

#define VSCP_REG_PAGE_SELECT_MSB            0x92
#define VSCP_REG_PAGE_SELECT_LSB            0x93

#define VSCP_REG_FIRMWARE_MAJOR_VERSION		0x94
#define VSCP_REG_FIRMWARE_MINOR_VERSION		0x95
#define VSCP_REG_FIRMWARE_SUB_MINOR_VERSION	0x96

#define VSCP_REG_BOOT_LOADER_ALGORITHM      0x97
#define VSCP_REG_BUFFER_SIZE                0x98
#define VSCP_REG_PAGES_USED                 0x99

// 32-bit
#define VSCP_REG_STANDARD_DEVICE_FAMILY_CODE 0x9A

// 32-bit
#define VSCP_REG_STANDARD_DEVICE_TYPE_CODE  0x9E

#define VSCP_REG_DEFAULT_CONFIG_RESTORE     0xA2

#define VSCP_REG_GUID                       0xD0
#define VSCP_REG_DEVICE_URL                 0xE0




#ifdef	__cplusplus
}
#endif

#endif	/* _VSCP_REGISTERS_H_ */


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
#include <stdint.h>
#include "systick.h"

#define MAXREADSPERCYCLE 4

static uint8_t data_size;
static char * user_data;
static uint8_t equal_count = 0;

void config_update (void);
void eeprom_write_local( unsigned int badd,unsigned char bdat );
unsigned char eeprom_read_local( unsigned int badd );
unsigned int eeprom_write_done(void);

void config_init (void * data, unsigned int size)
{
    if (size <= _EEPROMSIZE)
    {
        user_data = (char *) data; 
        data_size = size;
        for(int i = 0; i < size; i++)
        {
           user_data[i] = eeprom_read_local(i);                      
        }        
    }
    else
    {
        while(1);
    }    
    EECON1bits.WREN = 1;
    systick_register(config_update);
}

void config_wait_written (void)
{
    equal_count = 0;
    while (equal_count < data_size)
    {};
}

void config_update (void)
{
    static uint8_t offset = 0;
    uint8_t count = 0;
    
    if (!eeprom_write_done())
        return;
    
    while(count < MAXREADSPERCYCLE)
    {
        if(user_data[offset] != eeprom_read_local(offset))
        {
            eeprom_write_local(offset, user_data[offset]);
            return;
        }
        offset++;
        if (offset == data_size)
            offset = 0;
        if (equal_count < data_size)
            equal_count++;
        count++;
    }
}

void eeprom_write_local( unsigned int badd,unsigned char bdat )
{
	while(EECON1bits.WR);	       //Wait till the previous write completion
	EEADR = (badd & 0x0ff);
  	EEDATA = bdat;
  	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
}

unsigned char eeprom_read_local( unsigned int badd )
{
	while(EECON1bits.WR);	       //Wait till the previous write completion
	EEADR = (badd & 0x0ff);
  	EECON1bits.CFGS = 0;
	EECON1bits.EEPGD = 0;
	EECON1bits.RD = 1;
	while(EECON1bits.RD);          // Wait till read completes
	return ( EEDATA );             // return with read byte
}

unsigned int eeprom_write_done(void)
{
        return (EECON1bits.WR == 0);   
}

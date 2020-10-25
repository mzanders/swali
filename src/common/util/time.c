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

#include "time.h"
#include "systick.h"

/* Double buffer holding the value to be read */
static uint16_t current_time [2];

/* "get_time is going to read from here" */
static uint8_t read_pointer; 

/* "update has written valid data here" */
static uint8_t write_pointer; 

void time_update(void)
{
    static unsigned uint16_t counter = 0;
    uint8_t new_write_pointer = write_pointer;

    if (read_pointer == write_pointer)
    {
        new_write_pointer = (read_pointer + 1) % 2;
    }

    current_time [new_write_pointer] = ++counter;
    write_pointer = new_write_pointer;
}

void time_init(void)
{
    current_time[0] = 0;
    current_time[1] = 0;
    read_pointer = 0;
    write_pointer = 0;

    systick_register(time_update);
}

uint16_t time_get_ms(void)
{
    read_pointer = write_pointer;
    return current_time[read_pointer];
}

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

#include "systick.h"
#include <xc.h>
#include "led.h"
#define MAX_CALLBACKS 5

#define _XTAL_FREQ 40000000
#define TIMER0_RELOAD_VALUE (0xFFFF - (_XTAL_FREQ/4000))
static void (*Callback_List[MAX_CALLBACKS])();

void systick_initialize(void)
{
    for (int i = 0; i < MAX_CALLBACKS; i++)
    {
        Callback_List[i] = 0;
    }
    OpenTimer0(TIMER_INT_ON & T0_SOURCE_INT & T0_16BIT);
    WriteTimer0(TIMER0_RELOAD_VALUE);

}

void systick_register(void (*callback)(void))
{
    for (int i = 0; i < MAX_CALLBACKS; i++)
    {
        if (Callback_List[i] == 0)
        {
            Callback_List[i] = callback;
            return;
        }
    }
    while (1);
}

void systick_service(void)
{
    // Reload value for 1 ms resolution
    WriteTimer0(TIMER0_RELOAD_VALUE);

    // Call all the other functions which were registered.
    for (int i = 0; i < MAX_CALLBACKS; i++)
    {
        if (Callback_List[i] != 0)
            Callback_List[i]();
    }
}


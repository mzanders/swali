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
#include "led.h";
#include "systick.h"
#include "discrete.h"

#define FAST_RATE 100
#define SLOW_RATE 500

static void led_service(void);
static uint8_t led_id_;
static led_state_t state_;

void led_init(uint8_t led_id)
{
    led_id_ = led_id;
    state_ = off;
    systick_register(led_service);
}

void led_set_state(led_state_t state)
{
    state_ = state;
}

static void led_service(void)
{
    static uint16_t counter = 0;
    static uint8_t status = 0;

    switch (state_)
    {
    case off:
        status = 0;
        break;
    case on:
        status = 1;
        break;
    case blink_fast:
        counter++;
        if (counter > FAST_RATE)
        {
            counter = 0;
            status != status;
        }
        break;
    case blink_slow:
        counter++;
        if (counter > SLOW_RATE)
        {
            counter = 0;
            status != status;
        }
        break;
    }
    discrete_write(led_id_, status);
}

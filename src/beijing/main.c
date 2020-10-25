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
#include "systick.h"
#include "led.h"
#include "configuration.h"
#include "vscp.h"
#include "time.h"
#include "can.h"
#include "swali.h"
#include "discrete.h"
#include "pic_swali.h"

#pragma config WDT = OFF
#pragma config OSC = HSPLL
#pragma config PWRT = ON
#pragma config BOREN = BOACTIVE
#pragma config STVREN = ON
#pragma config BORV = 3
#pragma config LVP = OFF
#pragma config CPB = OFF
#pragma config WRTD  = OFF

#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF

#pragma config EBTRB = OFF

// function declarations
void init_platform(void);

// function definitions
int main()
{
    init_platform();
    systick_initialize();
    time_init();
    led_init(GREEN_LED_ID);
    initialize_config_data();
    can_init();
    vscp_init(vscp_message_handler, swali_event_handler);
    swali_init(config_swali, (uint8_t)(config_data_size - CONFIG_SWALI));
    while (1)
    {
        vscp_process(process_button());
        swali_process();  
    }
}

void init_platform(void)
{
    // Initialize the uP

    // Deselect comparators and ADC's so PORTA/PORTB
    // can be used as inputs.
    ADCON1 = 0x0f;

    // PORTA
    // RA0/AN0  - channel 9
    // RA1/AN1  - channel 8
    // RA2/AN2  - channel 7
    // RA3/AN3  - output
    // RA4      - output
    // RA5/AN4  - output
    TRISA = 0b00000111;
    PORTA = 0x00; // Default off

    // PortB

    // RB0/AN10     - Channel 0
    // RB1/AN8      - Channel 1
    // RB2 CAN TX   - output
    // RB3 CAN RX   - input
    // RB4/AN9      - Not used = output.
    // RB5/LVPGM    - Not used = output.
    // RB6/PGC      - Not used = output.
    // RB7/PGD      - Not used = output.
    TRISB = 0b00001011;
    PORTB = 0x00; // Default off

    // RC0 - Input  - Init. button
    // RC1 - Output - Status LED - Default off
    // RC2 - Output - Not used = output.
    // RC3 - Input - Channel 2.
    // RC4 - Input - Channel 3.
    // RC5 - Input - Channel 4.
    // RC6 - Input - Channel 5.
    // RC7 - Input - Channel 6.
    TRISC = 0b11111001;
    PORTC = 0x00; // Default off

    // Enable interrupts
    ei();

}

void interrupt interrupt_service(void)
{
    if (INTCONbits.TMR0IF)
    {
        systick_service();
        INTCONbits.TMR0IF = 0; // Clear Timer0 Interrupt Flag
    }
}

uint8_t discrete_read(uint8_t id)
{
    uint8_t rv = 0;
    switch (id)
    {
    case 0:
        rv = PORTBbits.RB0;
        break;
    case 1:
        rv = PORTBbits.RB1;
        break;
    case 2:
        rv = PORTCbits.RC3;
        break;
    case 3:
        rv = PORTCbits.RC4;
        break;
    case 4:
        rv = PORTCbits.RC5;
        break;
    case 5:
        rv = PORTCbits.RC6;
        break;
    case 6:
        rv = PORTCbits.RC7;
        break;
    case 7:
        rv = PORTAbits.RA2;
        break;
    case 8:
        rv = PORTAbits.RA1;
        break;
    case 9:
        rv = PORTAbits.RA0;
        break;
    case PUSHBUTTON_ID: // Pushbutton
        rv = PORTCbits.RC0;
        break;
    default:
        rv = 0;
        break;
    }
    return rv;
}

void discrete_write(uint8_t id, uint8_t value)
{
    switch (id)
    {
    case GREEN_LED_ID: // Green LED
        PORTCbits.RC1 = value;
        break;
    }
}
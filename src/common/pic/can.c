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

#include "can.h"
#include "ecan.h"

void can_init(void)
{
    ECANInitialize();
}

uint8_t can_send_extended(can_id_t id, uint8_t data[], uint8_t data_len)
{
    return ECANSendMessage(id, data, data_len, ECAN_TX_XTD_FRAME);
}

uint8_t can_receive_extended(can_id_t *id, uint8_t data[], uint8_t *data_len)
{
    ECAN_RX_MSG_FLAGS flags;
    uint8_t rv;

    rv = ECANReceiveMessage(id, data, data_len, &flags);
    // RTR not interesting
    if (flags & ECAN_RX_RTR_FRAME)
    {
        return 0;
    }

    // Must be extended frame
    if (!(flags & ECAN_RX_XTD_FRAME))
    {
        return 0;
    }
    return rv;
}

void can_add_rx_filter(can_id_t mask, can_id_t filter)
{
    static uint32_t the_mask = 0xffffffff;
    static uint32_t the_filter = 0x00000000;
    static uint8_t first_time = 1;

    if (first_time)
    {
        the_filter = filter;
        first_time = 0;
    }

    the_mask &= mask;

    the_mask &= ~(the_filter ^ filter);

    the_filter &= filter;

    // Must be in Config mode to change settings.
    ECANSetOperationMode(ECAN_OP_MODE_CONFIG);
    //Set mask 1
    ECANSetRXM1Value(the_mask, ECAN_MSG_XTD);
    // Set filter 1
    ECANSetRXF1Value(the_filter, ECAN_MSG_XTD);
    // Return to Normal mode to communicate.
    ECANSetOperationMode(ECAN_OP_MODE_NORMAL);


}



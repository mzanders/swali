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

#include "vscp.h"
#include "time.h"
#include "vscp_registers.h"
#include "can.h"

#define VSCP_MAJOR_VERSION 1
#define VSCP_MINOR_VERSION 9

/* Private variables */
/* Message callback to user application */
static void (*message_callback_) (vscp_message_t * message);
/* Event callback to the user application */
static void (*event_callback_) (vscp_event_t * event);

/* The state of the vscp node */
static uint8_t vscp_state;
/* Nickname of the vscp node */
static uint8_t nickname;

/* init state variables & constants */
/* timeout for probing an active device */
#define VSCP_PROBE_TIMEOUT 500
/* timeout for a master to come and give us an address */
#define VSCP_MASTER_TIMEOUT 5000
/* nickname currently in use for probing */
static uint8_t probe_nickname;
/* last time an action was performed (for timeouts) */
static uint16_t probe_start_time;
#define NUM_PROBE_RETRIES 3
static uint8_t probe_retry_count;

typedef enum
{
    send_probe, wait_for_ack, wait_for_master
} init_state_t;
init_state_t init_state;

/* active state variables & constants */
/* time in ms between node heartbeats */
#define VSCP_HEARTBEAT_PERIOD 60000
static uint16_t last_heartbeat;
static uint16_t vscp_current_page;
static uint8_t vscp_error_counter;

/* Private functions */
/* change the state to the indicated value. This calls the preparation of the
 state handler and notifies the user application through a message */
static void vscp_set_state(uint8_t State);

/* helper function for sending messages to the user application */
static void vscp_send_msg(uint8_t type, uint8_t length, uint8_t value[]);
/* helper for getting a value through a message to the user application */
static uint8_t vscp_get_msg_value(uint8_t type, uint8_t index, uint8_t * value);
static void vscp_set_msg_value(uint8_t type, uint8_t index, uint8_t value);
static uint8_t vscp_guid(uint8_t index);
static uint8_t vscp_mdf(uint8_t index);

/* send/receive any event to/from the CAN bus */
static void vscp_send_event(vscp_event_t * event);
static int vscp_receive_event(vscp_event_t * event);
static void vscp_process_protocol_event(vscp_event_t * event);
static void vscp_send_protocol_event(uint8_t type, uint8_t length, uint8_t data[]);

static uint8_t vscp_get_reg_value(uint8_t reg, uint16_t page);
static void vscp_set_reg_value(uint8_t reg, uint16_t page, uint8_t value);
static uint8_t vscp_get_reg_std_value(uint8_t reg);
static void vscp_set_reg_std_value(uint8_t reg, uint8_t value);
static uint8_t vscp_get_reg_msg_value(uint8_t reg, uint16_t page);
static void vscp_set_reg_msg_value(uint8_t reg, uint16_t page, uint8_t value);
static uint8_t vscp_std_reg_range(uint8_t reg);

/* vscp state prepare & handlers */
static void vscp_prepare_startup_state();
static void vscp_prepare_init_state();
static void vscp_prepare_active_state();
static void vscp_prepare_error_state();
static void vscp_handle_startup_state();
static void vscp_handle_init_state();
static void vscp_handle_active_state();
static void vscp_handle_error_state();

/* Function definitions */

void vscp_init(void message_callback(vscp_message_t * message),
               void event_callback(vscp_event_t * event))
{
    // Store the incoming callback functions for later use
    message_callback_ = message_callback;
    event_callback_ = event_callback;
    vscp_error_counter = 0;

    // set the internal state, initialize vscp_state
    vscp_set_state(VSCP_STATE_STARTUP);

}

void vscp_send(vscp_event_t * event)
{
    // User is not allowed to send protocol events
    // don't send anything when we're not in the active state
    if ((event->vscp_class != VSCP_CLASS1_PROTOCOL) &&
            (vscp_state == VSCP_STATE_ACTIVE))
        vscp_send_event(event);
}

// State processing & manipulation
// -------------------------------

void vscp_process(uint8_t init)
{
    if (init && (vscp_state != VSCP_STATE_INIT))
    {
        vscp_set_state(VSCP_STATE_INIT);
    }
    switch (vscp_state)
    {
    case VSCP_STATE_STARTUP:
        vscp_handle_startup_state();
        break;

    case VSCP_STATE_INIT:
        vscp_handle_init_state();
        break;

    case VSCP_STATE_ACTIVE:
        vscp_handle_active_state();
        break;

    case VSCP_STATE_ERROR:
        vscp_handle_error_state();
        break;
    }
    return;
}

static void vscp_set_state(uint8_t state)
{
    switch (state)
    {
    case VSCP_STATE_STARTUP:
        vscp_prepare_startup_state();
        break;

    case VSCP_STATE_INIT:
        vscp_prepare_init_state();
        break;

    case VSCP_STATE_ACTIVE:
        vscp_prepare_active_state();
        break;

    case VSCP_STATE_ERROR:
        vscp_prepare_error_state();
        break;
    }
    vscp_state = state;
    vscp_set_msg_value(VSCP_MSG_STATE, 0, state);
}

// STARTUP STATE
// =============

static void vscp_prepare_startup_state()
{
}

static void vscp_handle_startup_state()
{
    if (vscp_get_msg_value(VSCP_MSG_NICKNAME, 0, &nickname) &&
            (nickname != VSCP_NICKNAME_FREE))
        vscp_set_state(VSCP_STATE_ACTIVE);
    else
        vscp_set_state(VSCP_STATE_INIT);
}

// INIT STATE
// ==========

static void vscp_prepare_init_state()
{
    nickname = VSCP_NICKNAME_FREE;
    probe_nickname = VSCP_NICKNAME_MASTER;
    init_state = send_probe;
    probe_retry_count = 0;
}

static void vscp_handle_init_state()
{
    vscp_event_t rxevent;

    switch (init_state)
    {
    case send_probe:
        if (probe_nickname != VSCP_NICKNAME_FREE)
        {
            // Check if someone is on this address
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE, 1, &probe_nickname);
            init_state = wait_for_ack;
        }
        else
        {
            // Couldn't acquire an address, let everyone know we are stopping
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_PROBE_ACK, 1, &probe_nickname);
            vscp_set_msg_value(VSCP_MSG_NICKNAME, 0, VSCP_NICKNAME_FREE);
            vscp_set_state(VSCP_STATE_ERROR);
        }
        probe_start_time = time_get_ms();
        break;

    case wait_for_ack:
        // check for timeout first
        if ((time_get_ms() - probe_start_time) > VSCP_PROBE_TIMEOUT)
        {
            if (probe_retry_count == NUM_PROBE_RETRIES - 1)
            {
                probe_retry_count = 0;
                if (probe_nickname == VSCP_NICKNAME_MASTER)
                {
                    // No master present, move on to the next one
                    probe_nickname++;
                    init_state = send_probe;
                }
                else
                {
                    // Nobody replied on this address, current nickname is available
                    nickname = probe_nickname;
                    vscp_set_msg_value(VSCP_MSG_NICKNAME, 0, nickname);
                    vscp_set_state(VSCP_STATE_ACTIVE);
                }
            }
            else
            {
                init_state = send_probe;
                probe_retry_count++;
            }
        }
        else
        {
            if (vscp_receive_event(&rxevent))
            {
                if ((rxevent.vscp_class == VSCP_CLASS1_PROTOCOL) &&
                        (rxevent.vscp_type == VSCP_TYPE_PROTOCOL_PROBE_ACK) &&
                        (rxevent.size == 0) &&
                        (rxevent.nickname == probe_nickname))
                {
                    if (probe_nickname == VSCP_NICKNAME_MASTER)
                    {
                        // there is a master on the bus, let's wait for him
                        // to give us an address
                        probe_start_time = time_get_ms();
                        init_state = wait_for_master;
                    }
                    else
                    {
                        // someone has taken this address already, move on
                        probe_nickname++;
                        init_state = send_probe;
                    }
                }
            }
        }

        break;

    case wait_for_master:
        if ((time_get_ms() - probe_start_time) > VSCP_MASTER_TIMEOUT)
        {
            // Master hasn't come to give us an address, continue auto-discovery
            probe_nickname++;
            init_state = send_probe;
        }
        else
        {
            if (vscp_receive_event(&rxevent))
            {
                if ((rxevent.vscp_class == VSCP_CLASS1_PROTOCOL) &&
                        (rxevent.vscp_type == VSCP_TYPE_PROTOCOL_SET_NICKNAME) &&
                        (rxevent.size == 2) &&
                        (rxevent.data[0] == VSCP_NICKNAME_FREE))
                {
                    nickname = rxevent.data[1];
                    vscp_set_msg_value(VSCP_MSG_NICKNAME, 0, nickname);
                    vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED, 0, 0);
                    vscp_set_state(VSCP_STATE_ACTIVE);
                }
            }
        }
        break;
    }
}

// ACTIVE STATE
// ============

static void vscp_prepare_active_state()
{
    /* Let everyone know we're here */
    vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE, 1, &nickname);
    last_heartbeat = time_get_ms();
    vscp_current_page = 0;
}

static void vscp_handle_active_state()
{
    vscp_event_t tx_event;
    vscp_event_t rx_event;

    /* Send a periodic heartbeat */
    if ((time_get_ms() - last_heartbeat) > VSCP_HEARTBEAT_PERIOD)
    {
        tx_event.priority = VSCP_PRIORITY_LOW;
        tx_event.vscp_class = VSCP_CLASS1_INFORMATION;
        tx_event.vscp_type = VSCP_TYPE_INFORMATION_NODE_HEARTBEAT;
        tx_event.size = 3;
        tx_event.data[0] = 0;
        tx_event.data[1] = 0;
        tx_event.data[2] = 0;
        vscp_send_event(&tx_event);
        last_heartbeat = time_get_ms();
    }

    if (vscp_receive_event(&rx_event))
    {
        if (rx_event.vscp_class == VSCP_CLASS1_PROTOCOL)
        {
            vscp_process_protocol_event(&rx_event);
        }
        else
        {
            event_callback_(&rx_event);
        }
    }
}

// ERROR STATE
// ===========

static void vscp_prepare_error_state()
{
}

static void vscp_handle_error_state()
{
    // there's really nothing to do for VSCP except to wait for a reboot.
}

// Link to the CAN layer
// ---------------------

static void vscp_send_event(vscp_event_t * event)
{
    uint32_t id;
    uint16_t start_time;
    uint8_t error = 1;

    id = ((uint32_t) event->priority << 26) |
            ((uint32_t) event->vscp_class << 16) |
            ((uint32_t) event->vscp_type << 8) |
            nickname; // node address (our address)

    start_time = time_get_ms();

    // try for one second
    while ((time_get_ms() - start_time) < 1000)
    {
        if (can_send_extended(id, event->data, event->size))
        {
            error = 0;
            break;
        }
    }

    if (error)
    {
        vscp_error_counter++;
        if (vscp_error_counter == 0)
            vscp_error_counter--;
    }
}

static int vscp_receive_event(vscp_event_t * event)
{
    uint32_t id;

    if (can_receive_extended(&id, event->data, &(event->size)))
    {    
        event->nickname = (uint8_t) (id & 0x0ff);
        event->vscp_type = (uint8_t)((id >> 8) & 0xff);
        event->vscp_class = (uint8_t)((id >> 16) & 0x1ff);
        event->priority = (uint8_t) ((id >> 26) & 0x07);
        return 1;
    }
    return 0;
}

// VSCP protocol events processing
// -------------------------------

static void vscp_process_protocol_event(vscp_event_t * event)
{
    switch (event->vscp_type)
    {
    case VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE:
        if ((event->size == 1) && (event->data[0] == nickname))
        {
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_PROBE_ACK, 0, 0);
        }
        break;

    case VSCP_TYPE_PROTOCOL_SET_NICKNAME:
        if ((event->size == 2) && (event->data[0] == nickname))
        {
            nickname = event->data[1];
            vscp_set_msg_value(VSCP_MSG_NICKNAME, 0, nickname);
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED, 0, 0);
        }
        break;

    case VSCP_TYPE_PROTOCOL_DROP_NICKNAME:
        if ((event->size > 0) && (event->data[0] == nickname))
        {
            vscp_set_state(VSCP_STATE_INIT);
        }
        break;

    case VSCP_TYPE_PROTOCOL_READ_REGISTER:
        if ((event->size == 2) && (event->data[0] == nickname))
        {
            uint8_t txdata[2];
            txdata[0] = event->data[1];
            txdata[1] = vscp_get_reg_value(event->data[1], vscp_current_page);

            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_RW_RESPONSE,
                                     2,
                                     txdata);
        }
        break;

    case VSCP_TYPE_PROTOCOL_WRITE_REGISTER:
        if ((event->size == 3) && (event->data[0] == nickname))
        {
            vscp_set_reg_value(event->data[1], vscp_current_page, event->data[2]);
            // read back the register value so we're reporting the actual state
            event->data[2] = vscp_get_reg_value(event->data[1], vscp_current_page);
            // Note: reusing the incoming event buffer for transmission
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_RW_RESPONSE,
                                     2,
                                     &(event->data[1]));
        }
        break;

    case VSCP_TYPE_PROTOCOL_ENTER_BOOT_LOADER:
        if ((event->size == 8) && (event->data[0] == nickname))
        {
            uint8_t algorithm;
            if (vscp_get_msg_value(VSCP_MSG_BOOT_ALG, 0, &algorithm))
            {
                // the application supports bootloader
                if ((event->data[1] == algorithm) &&
                        (event->data[2] == vscp_guid(0)) &&
                        (event->data[3] == vscp_guid(3)) &&
                        (event->data[4] == vscp_guid(5)) &&
                        (event->data[5] == vscp_guid(7)))
                {
                    vscp_send_msg(VSCP_MSG_ENTER_BOOT, 0, 0);
                }
                else
                {
                    vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NACK_BOOT_LOADER, 1, &algorithm);
                }
            }
            else
            {
                vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_NACK_BOOT_LOADER, 1, &algorithm);
            }
        }
        break;

    case VSCP_TYPE_PROTOCOL_RESET_DEVICE:
        // not implementing this one due to difficult timing requirement
        // requires too many states to retain/initialize etc
        // limited practical use
        break;

    case VSCP_TYPE_PROTOCOL_INCREMENT_REGISTER:
    case VSCP_TYPE_PROTOCOL_DECREMENT_REGISTER:
        if ((event->size == 2) && (event->data[0] == nickname))
        {
            uint8_t txdata[2];
            txdata[0] = event->data[1];

            txdata[1] = vscp_get_reg_value(event->data[1], vscp_current_page);
            if (event->vscp_type == VSCP_TYPE_PROTOCOL_INCREMENT_REGISTER)
                txdata[1]++;
            else
                txdata[1]--;
            vscp_set_reg_value(event->data[1], vscp_current_page, txdata[1]);

            // reading back the actual value after increment/decrement
            txdata[1] = vscp_get_reg_value(event->data[1], vscp_current_page);

            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_RW_RESPONSE,
                                     2,
                                     txdata);
        }
        break;

    case VSCP_TYPE_PROTOCOL_WHO_IS_THERE:
        if ((event->size == 1) && ((event->data[0] == nickname) || (event->data[0] == 0xFF)))
        {
            // routine taken from original vscp_firmware file.
            uint8_t i, j, k;
            uint8_t data[8];

            k = 0;
            for (i = 0; i < 3; i++) // fill up with GUID
            {
                data[0] = i;
                
                for (j = 1; j < 8; j++)
                {
                    data[j] = vscp_guid(15 - k++);
                    if (k >= 16)
                        break;
                }
                if (k >= 16)
                    break;
                vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE, 8, data);
            }

            for (j = 0; j < 5; j++) // fill up previous event with MDF
            {
                if (vscp_mdf(j) > 0)
                    data[3 + j] = vscp_mdf(j);
                else
                    data[3 + j] = 0;
            }

            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE, 8, data);

            k = 5; // start offset
            for (i = 3; i < 7; i++) // fill up with the rest of GUID
            {
                data[0] = i;

                for (j = 1; j < 8; j++)
                {
                    data[j] = vscp_mdf(k++);
                }
                vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE, 8, data);
            }
        }
        break;

    case VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO:
        if ((event->size == 1) && (event->data[0] == nickname))
        {
            uint8_t data[4];
            for (uint8_t i = 0; i < 4; i++)
            {
                vscp_get_msg_value(VSCP_MSG_DMINFO, i, &data[i]);
            }
            vscp_send_protocol_event(VSCP_TYPE_PROTOCOL_GET_EMBEDDED_MDF_RESPONSE, 4, data);
        }
        break;

    case VSCP_TYPE_PROTOCOL_EXTENDED_PAGE_READ:
        if (event->data[0] == nickname)
        {
            uint16_t bytes;
            uint16_t byte = 0;
            uint8_t bytes_this_time;
            uint16_t vscp_page;

            vscp_event_t tx_event = {VSCP_PRIORITY_LOW,
                VSCP_CLASS1_PROTOCOL,
                VSCP_TYPE_PROTOCOL_EXTENDED_PAGE_RESPONSE,
                0,
                0,
                {0, 0, 0, 0, 0, 0, 0, 0}};

            // if data byte 4 of the request is present probably more than 1 register should be
            // read/written, therefore check lower 4 bits of the flags and decide
            if ((event->size) > 3)
            {
                // Number of registers was specified, thus take that value
                bytes = event->data[4];
                // if number of bytes was zero we read 256 bytes
                if (bytes == 0) bytes = 256;
            }
            else
            {
                bytes = 1;
            }

            // Compute the requested page
            vscp_page = ((uint16_t) (event->data[1] << 8) | (uint16_t) (event->data[2]));

            // Construct response event
            tx_event.data[0] = 0; // index of event, this is the first
            tx_event.data[1] = event->data[1]; // mirror page msb
            tx_event.data[2] = event->data[2]; // mirror page lsb

            do
            {
                // calculate bytes to transfer in this event
                if ((bytes - byte) >= 4)
                {
                    bytes_this_time = 4;
                }
                else
                {
                    bytes_this_time = (bytes - byte);
                }

                // define length of this event
                tx_event.size = 4 + bytes_this_time;
                tx_event.data[3] = event->data[3] + byte; // first register in this event

                // Put up to four registers to data space
                for (uint8_t cb = 0; cb < bytes_this_time; cb++)
                {
                    tx_event.data[ (4 + cb) ] =
                            vscp_get_reg_value((event->data[3] + byte + cb), vscp_page);
                }

                // send the event
                vscp_send_event(&tx_event);

                // increment byte by bytes_this_time and the event number by one
                byte += bytes_this_time;

                // increment the index
                tx_event.data[0] += 1;
            }
            while (byte < bytes);
        }
        break;

    case VSCP_TYPE_PROTOCOL_EXTENDED_PAGE_WRITE:
        if (event->data[0] == nickname)
        {
            uint16_t page;

            vscp_event_t tx_event = {VSCP_PRIORITY_LOW,
                VSCP_CLASS1_PROTOCOL,
                VSCP_TYPE_PROTOCOL_EXTENDED_PAGE_RESPONSE,
                0,
                0,
                {0, 0, 0, 0, 0, 0, 0, 0}};

            // Calculate the requested page
            page = (uint16_t) (event->data[1] << 8) | (uint16_t) (event->data[2]);

            for (uint8_t i = 0; i < (event->size - 4); i++)
            {
                vscp_set_reg_value((event->data[3] + i), page, event->data[4 + i]);
                tx_event.data[4 + i] = vscp_get_reg_value((event->data[3] + i), page);
            }

            tx_event.priority = VSCP_PRIORITY_LOW;
            tx_event.size = event->size;
            tx_event.data[0] = 0; // index of event, this is the first and only
            tx_event.data[1] = event->data[1]; // mirror page msb
            tx_event.data[2] = event->data[2]; // mirror page lsb
            tx_event.data[3] = event->data[3]; // Register

            // send the event
            vscp_send_event(&tx_event);
        }
        break;
    }
}

static void vscp_send_protocol_event(uint8_t type, uint8_t length, uint8_t data[])
{
    vscp_event_t tx_event = {VSCP_PRIORITY_HIGH, VSCP_CLASS1_PROTOCOL, 0, 0, 0,
        {0, 0, 0, 0, 0, 0, 0, 0}};

    tx_event.vscp_type = type;
    tx_event.size = length;

    for (int i = 0; i < length; i++)
    {
        tx_event.data[i] = data[i];
    }
    vscp_send_event(&tx_event);
}

// Register manipulation
// ---------------------

static uint8_t vscp_get_reg_value(uint8_t reg, uint16_t page)
{
    uint8_t value;

    if (reg < 0x80)
    {
        value = vscp_get_reg_msg_value(reg, page);
    }
    else
    {
        value = vscp_get_reg_std_value(reg);
    }

    return value;
}

static void vscp_set_reg_value(uint8_t reg, uint16_t page, uint8_t value)
{
    if (reg < 0x80)
    {
        vscp_set_reg_msg_value(reg, page, value);
    }
    else
    {
        vscp_set_reg_std_value(reg, value);
    }
    return;
}

static uint8_t vscp_std_reg_range(uint8_t reg)
{
    uint8_t value;

    value = reg;

    if ((reg >= VSCP_REG_USERID0) &&
            (reg <= VSCP_REG_USERID4))
        value = VSCP_REG_USERID0;

    if ((reg >= VSCP_REG_MANUFACTUR_ID0) &&
            (reg <= VSCP_REG_MANUFACTUR_SUBID3))
        value = VSCP_REG_MANUFACTUR_ID0;

    if ((reg >= VSCP_REG_STANDARD_DEVICE_FAMILY_CODE) &&
            (reg <= (VSCP_REG_STANDARD_DEVICE_TYPE_CODE + 3)))
        value = VSCP_REG_STANDARD_DEVICE_FAMILY_CODE;

    if ((reg >= VSCP_REG_GUID) &&
            (reg < (VSCP_REG_GUID + 16)))
        value = VSCP_REG_GUID;

    if (reg >= VSCP_REG_DEVICE_URL) // we're at the end of 256 bytes, no reason
        // to check for the upper boundary
        value = VSCP_REG_DEVICE_URL;

    return value;
}

static uint8_t vscp_get_reg_std_value(uint8_t reg)
{
    uint8_t value = 0;
    // to compress the switch statement below, the ranges of addresses are
    // checked in the above if statement
    switch (vscp_std_reg_range(reg))
    {
    case VSCP_REG_ALARMSTATUS:
        vscp_get_msg_value(VSCP_MSG_ALARMSTATUS, 0, &value);
        vscp_set_msg_value(VSCP_MSG_ALARMSTATUS, 0, 0);
        break;

    case VSCP_REG_VSCP_MAJOR_VERSION:
        value = VSCP_MAJOR_VERSION;
        break;

    case VSCP_REG_VSCP_MINOR_VERSION:
        value = VSCP_MINOR_VERSION;
        break;

    case VSCP_REG_NODE_ERROR_COUNTER:
        value = vscp_error_counter;
        break;

    case VSCP_REG_USERID0:
        vscp_get_msg_value(VSCP_MSG_USERID, (reg - VSCP_REG_USERID0), &value);
        break;

    case VSCP_REG_MANUFACTUR_ID0:
        vscp_get_msg_value(VSCP_MSG_MFGID, (reg - VSCP_REG_MANUFACTUR_ID0), &value);
        break;

    case VSCP_REG_NICKNAME_ID:
        value = probe_nickname;
        break;

    case VSCP_REG_PAGE_SELECT_MSB:
        value = (uint8_t) ((vscp_current_page >> 8) & 0x00FF);
        break;

    case VSCP_REG_PAGE_SELECT_LSB:
        value = (uint8_t) (vscp_current_page & 0x00FF);
        break;

    case VSCP_REG_FIRMWARE_MAJOR_VERSION:
    case VSCP_REG_FIRMWARE_MINOR_VERSION:
    case VSCP_REG_FIRMWARE_SUB_MINOR_VERSION:
        vscp_get_msg_value(VSCP_MSG_FWVERSION, (reg - VSCP_REG_FIRMWARE_MAJOR_VERSION), &value);
        break;

    case VSCP_REG_BOOT_LOADER_ALGORITHM:
        vscp_get_msg_value(VSCP_MSG_BOOT_ALG, 0, &value);
        break;

    case VSCP_REG_BUFFER_SIZE:
        value = 0;
        break;

    case VSCP_REG_PAGES_USED:
        vscp_get_msg_value(VSCP_MSG_PAGES_USED, 0, &value);
        break;

    case VSCP_REG_STANDARD_DEVICE_FAMILY_CODE: // 8 bytes!
        vscp_get_msg_value(VSCP_MSG_STD_DEVICE, (reg - VSCP_REG_STANDARD_DEVICE_FAMILY_CODE), &value);
        break;

    case VSCP_REG_GUID:
        value = vscp_guid(reg - VSCP_REG_GUID);
        break;

    case VSCP_REG_DEVICE_URL:
        value = vscp_mdf(reg - VSCP_REG_DEVICE_URL);
        break;

    }
    return value;
}

static void vscp_set_reg_std_value(uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case VSCP_REG_NODE_ERROR_COUNTER:
        vscp_error_counter = 0;
        break;

    case VSCP_REG_USERID0:
        vscp_set_msg_value(VSCP_MSG_USERID, (reg - VSCP_REG_USERID0), value);
        break;

    case VSCP_REG_PAGE_SELECT_MSB:
        vscp_current_page = (vscp_current_page & 0x00FF) | (value << 8);
        break;

    case VSCP_REG_PAGE_SELECT_LSB:
        vscp_current_page = (vscp_current_page & 0xFF00) | (value);
        break;

    case VSCP_REG_DEFAULT_CONFIG_RESTORE:
        // similar to VSCP_TYPE_PROTOCOL_RESET_DEVICE, this is not being
        // implemented due to the states and times to remember without bringing
        // lots of added value
        // it opens up a lot of problems if you're writing from two devices at
        // the same time etc etc
        break;
    }
    return;
}

static uint8_t vscp_get_reg_msg_value(uint8_t reg, uint16_t page)
{
    vscp_message_t message;
    uint8_t result;

    message.type = VSCP_GET | VSCP_MSG_REGVALUE;
    message.length = 3;
    message.value[0] = reg;
    message.value[1] = (uint8_t) ((page >> 8) & 0x00FF);
    message.value[2] = (uint8_t) (page & 0x00FF);

    message_callback_(&message);

    if (message.length == 4)
        result = message.value[3];
    else
        result = 0;

    return result;
}

static void vscp_set_reg_msg_value(uint8_t reg, uint16_t page, uint8_t value)
{
    vscp_message_t message;
    message.type = VSCP_SET | VSCP_MSG_REGVALUE;
    message.length = 4;
    message.value[0] = reg;
    message.value[1] = (uint8_t) ((page >> 8) & 0x00FF);
    message.value[2] = (uint8_t) (page & 0x00FF);
    message.value[3] = value;
    message_callback_(&message);
}

// helpers for exchanging messages with user applications
// ------------------------------------------------------

// unidirectional message

static void vscp_send_msg(uint8_t type, uint8_t length, uint8_t value[])
{
    vscp_message_t Message;

    if (length > MAX_MSG_DATA_LENGTH)
    {
        return;
    }

    Message.type = type;
    Message.length = length;
    for (int i = 0; i < length; i++)
    {
        Message.value[i] = value[i];
    }
    message_callback_(&Message);
}

// get a single byte from the user application identified by the type and index
// if the user application didn't handle the message, return 0 and value 0
// otherwise return 1 and the value provided by the application
// transmit message data length is 1 byte containing the index
// expected return is 2 bytes: index & the value
// if returned length is not 2 bytes, assume message wasn't handled

static uint8_t vscp_get_msg_value(uint8_t type, uint8_t index, uint8_t * value)
{
    vscp_message_t message;

    message.type = VSCP_GET | type;
    message.length = 1;
    message.value[0] = index;

    message_callback_(&message);

    if ((message.value[0] == index) && (message.length == 2))
    {
        *value = message.value[1];
        return 1;
    }

    value = 0;
    return 0;
}

// set a single byte in the user application indicated by the type and the index
// don't care if the user handles the message or not.

static void vscp_set_msg_value(uint8_t type, uint8_t index, uint8_t value)
{
    vscp_message_t message;
    message.type = VSCP_SET | type;
    message.length = 2;
    message.value[0] = index;
    message.value[1] = value;
    message_callback_(&message);
}

static uint8_t vscp_guid(uint8_t index)
{
    uint8_t value;
    vscp_get_msg_value(VSCP_MSG_GUID, index, &value);
    return value;
}

static uint8_t vscp_mdf(uint8_t index)
{
    uint8_t value;
    vscp_get_msg_value(VSCP_MSG_MDF, index, &value);
    return value;
}

#include "swali_config.h"
#include "swali.h"
#include "discrete.h"
#include "vscp.h"
#include "swali_output.h"
#include "time.h"

static void send_info_event(swali_output_data_t * data);
static void send_control_event(swali_output_data_t * data);
static void update_output(swali_output_data_t * data);
static void write_flag(swali_output_data_t * data, uint8_t flag, uint8_t value);
static uint8_t read_flag(swali_output_data_t * data, uint8_t flag);
static uint8_t reg_range(uint8_t reg);
static uint8_t timer_on(swali_output_data_t * data);

#define FLAG_ENABLE       0x80
#define FLAG_INVERT       0x20

#define REG_ID0           0x00 // read only
#define REG_ID1           0x01 // read only
#define REG_STATE         0x02 // read only
#define REG_ENABLE        0x03 // R/W
#define REG_ZONE          0x04 // R/W
#define REG_SUBZONE       0x05 // R/W
#define REG_INVERT        0x07 // R/W  1 = invert
#define REG_ON_TIME_HRS   0x08 // R/W
#define REG_ON_TIME_MINS  0x09 // R/W  HRS==0 && MINS==0 > no timer
#define REG_ACT_TIME_HRS  0x0A // R
#define REG_ACT_TIME_MINS 0x0B // R
#define REG_NAME          0x10 // R/W max 16chars

void swali_output_initialize(uint8_t swali_channel, swali_output_config_t * config, swali_output_data_t * data)
{
    data->swali_channel = swali_channel;
    data->config = config;
    data->state = 0;
    data->last_state = 0;
    data->last_time_ms = time_get_ms();
    data->on_time_hrs = 0;
    data->on_time_mins = 0;
    update_output(data);
}

void swali_output_process(swali_output_data_t * data)
{
    uint16_t current_time = time_get_ms();

    // channel not enabled? Return!
    if (!(data->config->flags & FLAG_ENABLE))
    {
        return;
    }

    // process the actual on timers
    if (data->state)
    {
        if ((current_time - data->last_time_ms) > 60000)
        {
            //minute has elapsed
            data->on_time_mins++;
            if (data->on_time_mins == 60)
            {
                data->on_time_hrs++;
                data->on_time_mins = 0;
            }
            data->last_time_ms = current_time;
        }
    }
    else
    {
        data->last_time_ms = current_time;
        data->on_time_hrs = 0;
        data->on_time_mins = 0;
    }

    /* timer expired, send an event to turn off all outputs */
    if (timer_on(data) &&
            (data->on_time_hrs == data->config->on_time_hrs) &&
            (data->on_time_mins == data->config->on_time_mins))
    {
        send_control_event(data);
    }

    // internal state changed (incoming event/timer), 
    // send an information event
    if (data->last_state != data->state)
    {
        send_info_event(data);
    }

    update_output(data);
    data->last_state = data->state;
}

void swali_output_handle_event(swali_output_data_t * data, vscp_event_t * event)
{
    // channel not enabled? Return!
    if (!(data->config->flags & FLAG_ENABLE))
        return;

    // output channels get to process control events
    if ((event->vscp_class == VSCP_CLASS1_CONTROL) &&
            (event->size == 3) &&
            (data->config->zone == event->data[1]) &&
            ((data->config->subzone == event->data[2]) ||
            (255 == event->data[2])))
    {
        if (event->vscp_type == VSCP_TYPE_CONTROL_TURNOFF)
        {
            if (data->state == 1)
            {
                data->state = 0; // switch the state off
            }
            else
            {
                // a slave was not synchronized, this ensures an info update
                // is sent out on the next process cycle
                data->last_state = 1;
            }

        }
        if (event->vscp_type == VSCP_TYPE_CONTROL_TURNON)
        {
            if (data->state == 0)
            {
                data->state = 1; // switch the state on
            }
            else
            {
                // a slave was not synchronized, this ensures an info update
                // is sent out on the next process cycle
                data->last_state = 0;
            }
        }
    }
}

void swali_output_write_reg(swali_output_data_t * data, uint8_t reg, uint8_t value)
{
    switch (reg_range(reg))
    {
    case REG_ENABLE:
        write_flag(data, FLAG_ENABLE, value);
        break;
    case REG_ZONE:
        data->config->zone = value;
        break;
    case REG_SUBZONE:
        data->config->subzone = value;
        break;
    case REG_INVERT:
        write_flag(data, FLAG_INVERT, value);
        break;
    case REG_ON_TIME_HRS:
        data->config->on_time_hrs = value;
        break;
    case REG_ON_TIME_MINS:
        data->config->on_time_mins = value;
        break;
    case REG_NAME:
        if (((reg - REG_NAME) < SWALI_NAME_LENGTH) && ((reg - REG_NAME) < 16))
            data->config->name[reg - REG_NAME] = value;
    }
}

uint8_t swali_output_read_reg(swali_output_data_t * data, uint8_t reg)
{
    uint8_t value = 0;

    switch (reg_range(reg))
    {
    case REG_ID0:
        value = 'O';
        break;
    case REG_ID1:
        value = 'U';
        break;
    case REG_STATE:
        value = data->state;
        break;
    case REG_ENABLE:
        value = read_flag(data, FLAG_ENABLE);
        break;
    case REG_ZONE:
        value = data->config->zone;
        break;
    case REG_SUBZONE:
        value = data->config->subzone;
        break;
    case REG_INVERT:
        value = read_flag(data, FLAG_INVERT);
        break;
    case REG_ON_TIME_HRS:
        value = data->config->on_time_hrs;
        break;
    case REG_ON_TIME_MINS:
        value = data->config->on_time_mins;
        break;
    case REG_ACT_TIME_HRS:
        value = data->on_time_hrs;
        break;
    case REG_ACT_TIME_MINS:
        value = data->on_time_mins;
        break;
    case REG_NAME:
        if (((reg - REG_NAME) < SWALI_NAME_LENGTH) && ((reg - REG_NAME) < 16))
            value = data->config->name[reg - REG_NAME];
        break;
    }
    return value;
}

static void send_control_event(swali_output_data_t * data)
{
    vscp_event_t tx_event;

    tx_event.priority = VSCP_PRIORITY_MEDIUM;
    tx_event.vscp_class = VSCP_CLASS1_CONTROL;
    tx_event.size = 3;
    tx_event.data[0] = data->swali_channel; // optional;
    tx_event.data[1] = data->config->zone; // all zones;
    tx_event.data[2] = data->config->subzone; // all subzones;

    tx_event.vscp_type = VSCP_TYPE_CONTROL_TURNOFF;

    swali_send_event(&tx_event);
}

static void send_info_event(swali_output_data_t * data)
{
    vscp_event_t tx_event;
    // send on or off control event
    tx_event.priority = VSCP_PRIORITY_MEDIUM;
    tx_event.vscp_class = VSCP_CLASS1_INFORMATION;
    if (data->state)
        tx_event.vscp_type = VSCP_TYPE_INFORMATION_ON;
    else
        tx_event.vscp_type = VSCP_TYPE_INFORMATION_OFF;
    tx_event.size = 3;
    tx_event.data[0] = data->swali_channel;
    tx_event.data[1] = data->config->zone;
    tx_event.data[2] = data->config->subzone;

    swali_send_event(&tx_event);
}

static void update_output(swali_output_data_t * data)
{
    if (data->config->flags & FLAG_INVERT)
        discrete_write(data->swali_channel, !data->state);
    else
        discrete_write(data->swali_channel, data->state);
}

static void write_flag(swali_output_data_t * data, uint8_t flag, uint8_t value)
{
    if (value == 0)
        data->config->flags &= ~flag;
    else
        data->config->flags |= flag;
}

static uint8_t read_flag(swali_output_data_t * data, uint8_t flag)
{
    if (data->config->flags & flag)
        return 1;
    else
        return 0;
}

static uint8_t reg_range(uint8_t reg)
{
    uint8_t value = reg;

    if ((reg >= REG_NAME) &&
            (reg < REG_NAME + 16))
        value = REG_NAME;

    return value;
}

static uint8_t timer_on(swali_output_data_t * data)
{
    return ((data->config->on_time_hrs > 0) || (data->config->on_time_mins > 0));
}

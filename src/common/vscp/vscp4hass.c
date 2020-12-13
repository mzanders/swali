#include "vscp4hass.h"


const vscp4hass_bs_class_t vscp4hass_bs_class_map[] = {
            {0x14, 0x03, 0x04 }, // 0x00 - generic
            {0x14, 0x0A, 0x0B }, // 0x01 - battery
            {0x14, 0x03, 0x04 }, // 0x02 - battery_charging
            {0x02, 0x0C, 0x8C }, // 0x03 - cold TBC
            {0x14, 0x51, 0x52 }, // 0x04 - connectivity
            {0x02, 0x09, 0x89 }, // 0x05 - door TBC
            {0x14, 0x07, 0x08 }, // 0x06 - garage_door
            {0x02, 0x03, 0x04 }, // 0x07 - gas TBC
            {0x02, 0x07, 0x87 }, // 0x08 - heat TBC
            {0x14, 0x03, 0x04 }, // 0x08 - light
            {0x14, 0x4B, 0x4C }, // 0x09 - lock
            {0x02, 0x10, 0x90 }, // 0x0A - moisture TBC
            {0x02, 0x01, 0x81 }, // 0x0B - motion TBC
            {0x14, 0x03, 0x04 }, // 0x0C - moving
            {0x14, 0x54, 0x55 }, // 0x0D - occupancy
            {0x14, 0x07, 0x08 }, // 0x0E - opening
            {0x14, 0x03, 0x04 }, // 0x0F - plug
            {0x14, 0x03, 0x04 }, // 0x10 - power
            {0x14, 0x54, 0x55 }, // 0x11 - presence
            {0x14, 0x29, 0x92 }, // 0x12 - problem TBC
            {0x02, 0x00, 0x80 }, // 0x13 - safety TBC
            {0x02, 0x06, 0x86 }, // 0x14 - smoke TBC
            {0x02, 0x12, 0x92 }, // 0x15 - sound TBC
            {0x02, 0x05, 0x85 }, // 0x16 - vibration TBC
            {0x02, 0x0A, 0x8A }  // 0x17 - window TBC
            };
# VSCP4HASS - framework to integrate VSCP nodes easily into Home Assistant

https://www.vscp.org
https://www.home-assistant.io

## Objective
VSCP4HASS defines a standardized interface to VSCP nodes to implement generic
entities that are defined in HASS. This allows easy expansion of a VSCP4HASS
network without any configuration change on the HASS side.

The goals of VSCP4HASS are:
- all configuration data is contained in the nodes, no separate config files are
  required to describe/interface the system
- HASS doesn't need to look up register addresses in the VSCP MDF
- everything can be discovered on the initiative of HASS
- changes on the VSCP network are automatically propagated to HASS
- The VSCP event matching the most with the targeted function is used where
  possible and practical.

VSCP4HASS uses asyncio interfaces exclusively. It uses an external Python module
to communicate with a vscpd compatible instance.

## VSCP features used
The following features of VSCP are used in order to meet the objectives above:
- "who's there" events are used to scan the bus and gather GUID's.
- Extended page register reads are used to read the device registers.
- The standard device family code (starting address 0x9B) is set to "HASS" in
  UTF-8 encoding, this allows VSCP4HASS to scan a bus looking for compatible
  devices. Standard device type is set to all 0's and reserved for future use.
- The number of pages in a VSCP4HASS node (register address 0x99 - deprecated by
  VSCP) identifies the number of channels available on that node. Zero based.
- Each channel is mapped to a single page of registers, starting from 0x0000.
- The GUID for each node together with the channel number acts as the unique ID
  towards HASS.
- Node heartbeats (CLASS1.Information, type 0x09) are used to determine if a
  node is still connected.
- New node online messages (CLASS1.Protocol, type 0x02) are used to enable new
  or reconfigure existing nodes (ie after an external configuration change, a
  reset should be forced on the node to refresh the status in HASS).

The first 32 (0x20) registers for each channel/page are defined for use by
VSCP4HASS. The remainder of the page is available to implement other
functionalities. VSCP4HASS considers all registers as read-only. Configuration
of the registers (using the MDF) has to happen with external tools.

The first 2 registers of each channel act as an entity identifier, encoded as a
2 byte UTF-8 string.
Next is an enable register. If the channel is disabled, it will not be loaded
into HASS.
Further registers up to 0x0F define entity-dependent behaviour.
Each channel can optionally have a configured name (max 16 chars), depending on
available non-volatile memory capacity in the node.

The remaining registers (0x20-0x7F) are free to be used by the implementer.
For instance when implementing a binary sensor which receives push button
status, one might have it automatically send turn on/off events to a light. As
VSCP4HASS has no need for the zone/subzone to control the light from the button,
this information should be contained in the registers above 0x20 for that
channel. Other examples are polarity inversion, built-in timers etc.

## Home Assistant entities definitions
### Light
VSCP registers:
    0x00-0x01: Identifier: "LI"
    0x02: Enable (0=disabled)
    0x03: capabilities flags
      encoded as (currently identical as in HASS):
      * 0x01: support brightness
      * 0x02: support color temperature (NOT IMPLEMENTED, always 0)
      * 0x04: support effect (NOT IMPLEMENTED, always 0)
      * 0x08: support flash
      * 0x10: support color (NOT IMPLEMENTED, always 0)
      * 0x20: support transition (NOT IMPLEMENTED, always 0)
      * 0x40: support white value (NOT IMPLEMENTED, always 0)
    0x04: State (1=on)
    0x05: VSCP zone for this light
    0x06: VSCP subzone for this light
    0x07: brightness value, 0-255
    0x08-0x0F: reserved for future use
    0x10-0x1F: light name, null terminated (all 0's if not used)

VSCP events:
    CLASS1.CONTROL, 0x1E - Type=0x05: TurnOn
      Sent from HASS to the zone/subzone (from the registers) to turn on, data
      byte 0 indicates flashing mode: 0=no flashing, 1=short, 2=long
    CLASS1.CONTROL, 0x1E - Type=0x07: TurnOff
      Sent from HASS to the zone/subzone (from registers) to turn off, data
      byte 0 not used
    CLASS1.CONTROL, 0x1E - Type=0x16: Change level
      Sent from HASS to the zone/subzone to set dimmer value
      Note: this is used as opposed to the command "Dim lamp(s)"
    CLASS1.INFORMATION, 0x14 - Type=0x03: On
      Sent from the VSCP node when the light turns on. Data byte 0 is the
      channel number of the VSCP node. Zone/subzone is ignored.
    CLASS1.INFORMATION, 0x14 - Type=0x04 Off
      Sent from the VSCP node when the light turns off. Data byte 0 is the
      channel number of the VSCP node. Zone/subzone is ignored.

### Binary sensor
VSCP registers:
    0x00-0x01: Identifier: "BS"
    0x02: Enabled (0=disable)
    0x03: State (1=on)
    0x04: Class ID
    0x05-0x0F: reserved for future use
    0x10-0x1F: binary sensor name, null terminated (all 0's if not used)

VSCP events:
    The CLASS1.Information events from the table below are used for the
    respective class ID's. Byte 0 always indicates which channel of the node
    is reporting the status. Zone/subzone information is ignored in VSCP4HASS
    and can be used for other purposes.

Class ID - Class - VSCP event mapping - Work-in-progress

| Class ID | HASS Device Class | VSCP Class  | ON event type       | OFF event type     |
|----------|-------------------|-------------|-------------------- |--------------------|
| 0x00     | generic           | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x01     | battery           | 0x20 - info | 0x0A - Below limit  | 0x0B - Above limit |
| 0x02     | battery_charging  | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x03     | cold              | 0x02 - secu | 0x0C - Frost        | ???                |
| 0x04     | connectivity      | 0x20 - info | 0x51 - Connect      | 0x52 - Disconnect  |
| 0x05     | door              | 0x02 - secu | 0x09 - Door contact | ???                |
| 0x06     | garage_door       | 0x20 - info | 0x07 - Opened       | 0x08 - Closed      |
| 0x07     | gas               | 0x20 - info | 0x0B - Above limit  | 0x0A - Below limit |
| 0x08     | heat              | 0x02 - secu | 0x07 - Heat sensor  | ???                |
| 0x08     | light             | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x09     | lock              | 0x20 - info | 0x4B - Lock         | 0x4C - Unlock      |
| 0x0A     | moisture          | 0x02 - secu | 0x10 - Water        | ???                |
| 0x0B     | motion            | 0x02 - secu | 0x01 - Motion       | ???                |
| 0x0C     | moving            | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x0D     | occupancy         | 0x20 - info | 0x54 - Enter        | 0x55 - Exit        |
| 0x0E     | opening           | 0x20 - info | 0x07 - Opened       | 0x08 - Closed      |
| 0x0F     | plug              | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x10     | power             | 0x20 - info | 0x03 - ON           | 0x04 - OFF         |
| 0x11     | presence          | 0x20 - info | 0x54 - Enter        | 0x55 - Exit        |
| 0x12     | problem           | 0x20 - info | 0x29 - Warning      | ???                |
| 0x13     | safety            | 0x02 - secu | 0x00 - Generic      | ???                |
| 0x14     | smoke             | 0x02 - secu | 0x06 - Smoke sensor | ???                |
| 0x15     | sound             | 0x02 - secu | 0x12 - Noise        | ???                |
| 0x16     | vibration         | 0x02 - secu | 0x05 - Shock        | ???                |
| 0x17     | window            | 0x02 - secu | 0x0A - Window       | ???                |


### Sensor
VSCP registers:
    0x00-0x01: Identifier: "SE"
    0x02: Enabled (0=disable)
    0x03: Class ID
    0x04: Sensor ID
    0x05: Generic
    0x10-0x1F: binary sensor name, null terminated (all 0's if not used)

VSCP events:
    The CLASS1.MEASUREZONE is used to transmit measurement data. Byte 0 always
    indicates	which channel of the node is reporting the value. Byte 1/2
    (zone/subzone) is ignored	by VSCP4HASS. The datacoding in byte 3 has 2 4bit
    nibbles:
	bit 0-3: VSCP unit ID for the value
	bit 4-7: Value encoding format as in VSCP:
				0b0000: set of bits
				0b0001: byte
				0b0010: string (only 4chars..)
				0b0011: signed integer
				0x0100: normalized signed integer
				0x0101: 32bit IEE754 float
				0x1011: unsigned integer
				0x1100: normalized unsigned integer			

Class ID mapping - Work-in-progress

| Class ID | HASS device Class |
|----------|-------------------|
| 0x00     | generic           |    
| 0x01     | humidity          |    
| 0x02     | illuminance       |        
| 0x03     | signal_strength   |            
| 0x04     | temperature       |        
| 0x05     | timestamp         |      
| 0x06     | power             |  
| 0x07     | pressure          |    
| 0x08     | current           |    
| 0x09     | energy            |  
| 0x0A     | power_factor      |        
| 0x0B     | voltage           |  

import struct

from .channel import Channel

from .vscp.event import Event
from .vscp.const import (CLASS_CONTROL, CLASS_INFORMATION,
                         EVENT_INFORMATION_ON, EVENT_INFORMATION_OFF,
                         EVENT_CONTROL_TURN_ON,
                         EVENT_CONTROL_TURN_OFF)

from .vscp.util import read_reg

IDENTIFIER = 'OU'

class Light(Channel):
    @classmethod
    def identifier(cls):
        return IDENTIFIER

    @classmethod
    def events(cls):
        return [(CLASS_INFORMATION, EVENT_INFORMATION_ON),
                (CLASS_INFORMATION, EVENT_INFORMATION_OFF)]

    @classmethod
    async def handle_event(cls, channels, event):
        if event.vscp_type == EVENT_INFORMATION_ON and len(event.data) == 3:
            nick = event.guid.nickname
            channel = event.data[0]
            if((nick, channel) in channels):
                await channels[(nick, channel)].update(True)

        if event.vscp_type == EVENT_INFORMATION_OFF and len(event.data) == 3:
            nick = event.guid.nickname
            channel = event.data[0]
            if((nick, channel) in channels):
                await channels[(nick, channel)].update(False)

    @classmethod
    async def new(cls, bus, nickname, channel):
        self = cls()
        self.nickname = nickname
        self.channel = channel
        self.bus = bus
        self.callback = list()

        registers = await read_reg(bus, nickname, channel, 0, 34)
        self.state = (registers[2] != 0x00)
        self.enabled = (registers[3] != 0x00)
        self.zone = int(registers[4])
        self.subzone = int(registers[5])
        self.name = registers[16:33].decode().rstrip('/x0')
        return self

    async def update(self, state):
        self.state = state
        for c in self.callback:
            await c(self, self.state)

    async def set(self, state):
        if state:
            type = EVENT_CONTROL_TURN_ON
        else:
            type = EVENT_CONTROL_TURN_OFF
        ev = Event(vscp_class=CLASS_CONTROL,
                   vscp_type=type,
                   data=struct.pack('>BBB', 0, self.zone, self.subzone))
        await self.bus.send(ev)

    def add_callback(self, callback):
        self.callback.append(callback)

    def group(self):
        return (self.zone, self.subzone)

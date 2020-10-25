from .channel import Channel
from .vscp.const import (CLASS_INFORMATION, EVENT_INFORMATION_BUTTON)
from .vscp.util import read_reg

IDENTIFIER = 'IN'

class Switch(Channel):
    @classmethod
    def identifier(cls):
        return IDENTIFIER

    @classmethod
    async def handle_event(cls, channels, event):
        if event.vscp_type == EVENT_INFORMATION_BUTTON and len(event.data) == 5:
            nick = event.guid.nickname
            channel = event.data[4]
            if((nick, channel) in channels):
                await channels[(nick, channel)].update(event.data[0] != 0x01)

    @classmethod
    def events(cls):
        return [(CLASS_INFORMATION, EVENT_INFORMATION_BUTTON)]

    @classmethod
    async def new(cls, bus, nickname, channel):
        self = cls()
        self.nickname = nickname
        self.channel = channel
        self.bus = bus
        self.callback = list()

        registers = await read_reg(bus, nickname, channel, 0, 6)
        self.state = (registers[2] != 0x00)
        self.enabled = (registers[3] != 0x00)
        self.zone = int(registers[4])
        self.subzone = int(registers[5])
        return self

    async def update(self, state):
        self.state = state
        for c in self.callback:
            await c(self, self.state)

    def add_callback(self, callback):
        self.callback.append(callback)

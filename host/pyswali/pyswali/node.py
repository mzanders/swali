from .vscp.const import (STD_REG_STD_DEV,
                         STD_REG_PAGES,
                         STD_REG_UID)
from .vscp.util import write_reg, read_reg, read_std_reg
from .channel import channel_reg

CHANNEL_TYPE = 0
CHANNEL_TYPE_SIZE = 2

class Node:
    @classmethod
    async def new(cls, bus, nickname, guid=None, mdf=None):
        self = cls()
        self.bus = bus
        self.nickname = nickname
        self.guid = guid
        self.uid = await read_std_reg(bus, nickname, STD_REG_UID)
        self.pages = int((await read_std_reg(bus, nickname, STD_REG_PAGES))[0])
        self.stddev = await read_std_reg(bus, nickname, STD_REG_STD_DEV)
        self.is_swali = True if self.stddev == b'SWALI\0\0\0' else False

        if self.is_swali:
            self.channels = dict()
            for channel_type in channel_reg:
                self.channels[channel_type] = dict()

            for channel in range(self.pages):
                channel_type = (await read_reg(bus, nickname, channel, CHANNEL_TYPE, num=CHANNEL_TYPE_SIZE)).decode("utf-8")
                if channel_type in channel_reg:
                    self.channels[channel_type][(nickname, channel)] = \
                        await channel_reg[channel_type].new(bus, nickname, channel, guid)

        return self

    async def set_uid(self, uid):
        if not isinstance(uid, (bytes, bytearray)) or (len(uid) != 5):
            raise ValueError('Incorrect UID')

        for i, value in enumerate(uid):
            await write_reg(self.bus, 0, 0x84+i, self.nickname, value)
        self.uid = bytes(uid)

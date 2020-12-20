from vscp.util import write_reg, read_reg
from vscp.const import STD_REG_FW_MAJOR, STD_REG_MDF, STD_REG_GUID
from vscp.guid import Guid

class Version:
    def __init__(self, raw):
        self.major = int(raw[0])
        self.minor = int(raw[1])
        self.micro = int(raw[2])

    def __repr__(self):
        return '{:02}.{:02}.{:02}'.format(self.major, self.minor, self.micro)


class Node:
    def __init__(self, gw, nick, guid=None, mdf=None):
        self.gw = gw
        self.nick = nick
        self.channels = list()
        self.version = None
        self.mdf = mdf
        self.guid = guid

    async def init(self):
        self.version = Version(await read_reg(self.gw, self.nick, 0, STD_REG_FW_MAJOR, 3))
        if self.mdf is None:
            self.mdf = (await read_reg(self.gw, self.nick, 0, STD_REG_MDF, 32)).decode().rstrip('/x0')
        if self.guid is None:
            self.guid = Guid(await read_reg(self.gw, self.nick, 0, STD_REG_GUID, 16))

    async def read_reg(self, channel, reg, num=1):
        return await read_reg(self.gw, self.nick, channel, reg, num)

    async def write_reg(self, channel, reg, value):
        await write_reg(self.gw, channel, reg, self.nick, value)

    async def menu(self):
        print('GUID: {}'.format(self.guid))
        while True:
            print('Select channel, b to enter bootloader, q to quit.  > ')
            for i, channel in enumerate(self.channels):
                name = await channel.name()
                print(' {:3} - {} {}'.format(i, type(channel).__name__, name))
            ui = input('> ')
            if ui == 'q':
                break
            elif ui == 'b':
                pass
            else:
                try:
                    await self.channels[int(ui)].menu()
                except IndexError:
                    print('Invalid channel, try again!')

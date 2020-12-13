from vscp.util import write_reg, read_reg


class Node:
    def __init__(self, gw, nick):
        self.gw = gw
        self.nick = nick

    async def read_reg(self, channel, reg, num=1):
        return await read_reg(self.gw, self.nick, channel, reg, num)

    async def write_reg(self, channel, reg, value):
        await write_reg(self.gw, channel, reg, self.nick, value)

    async def menu(self):
        while True:
            print('Select channel (q to quit):')
            for i, channel in enumerate(self.channels):
                name = await channel.name()
                print(' {:3} - {} {}'.format(i, type(channel).__name__, name))
            ui = input('> ')
            if ui == 'q':
                break

            try:
                await self.channels[int(ui)].menu()
            except IndexError:
                print('Invalid channel, try again!')

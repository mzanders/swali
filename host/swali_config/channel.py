import struct
import asyncio


class Channel:
    async def menu(self):
        while True:
            print('Reading channel registers...')
            reg_data = await self.node.read_reg(self.index, 0, self.num_registers)
            print('Select a register to change or q to quit.')
            for reg, (descr, writeable) in self.reglist.items():
                if descr == 'Name':
                    val = self._get_name(reg_data)
                else:
                    val = int(reg_data[reg])
                print(' {:2} - {:18}{}: {}'.format(reg, descr, ' ' if writeable else 'R', val))

            ui = input('> ')
            if ui == 'q':
                break
            reg = int(ui)

            try:
                if self.reglist[reg][0] == 'Name':
                    ui = input('New name? >')
                    name = ui.encode('UTF-8')
                    name = bytearray(name) + b'\00' * (16-len(name))
                    for i in range(0,16,4):
                        await self.node.write_reg(self.index, reg+i, name[i:i+4])
                else:
                    val = struct.pack('B', int(input('New value? >')))
                    await self.node.write_reg(self.index, reg, val)
            except (KeyError, IndexError):
                print('Wrong input, try again!')
            await asyncio.sleep(0.1)

class Light(Channel):
    def __init__(self, node, index):
        self.node = node
        self.index = index
        self.reglist = {0x02: ('Enable', True),
                        0x04: ('State', True),
                        0x05: ('Zone', True),
                        0x06: ('Subzone', True),
                        0x10: ('Name', True),
                        0x20: ('On time hrs', True),
                        0x21: ('On time mins', True),
                        0x22: ('Act on time hrs', False),
                        0x23: ('Act on time mins', False),
                        0x24: ('Invert', True)}
        self.num_registers = 40

    @staticmethod
    def _get_name(registers):
        return registers[16:32].decode().rstrip('/x0')

    async def name(self):
        reg_data = await self.node.read_reg(self.index, 0x10, 0x10)
        return reg_data.decode().rstrip('/x0')

class Switch(Channel):
    def __init__(self, node, index):
        self.node = node
        self.index = index
        self.reglist = {0x02: ('Enable', True),
                        0x03: ('State', False),
                        0x04: ('Class ID', True),
                        0x10: ('Name', True),
                        0x20: ('Zone', True),
                        0x21: ('Subzone', True),
                        0x22: ('Type', True),
                        0x23: ('Invert', True),
                        0x24: ('ON flash type', True)}
        self.num_registers = 40

    @staticmethod
    def _get_name(registers):
        return registers[16:32].decode().rstrip('/x0')

    async def name(self):
        reg_data = await self.node.read_reg(self.index, 0x10, 0x10)
        return reg_data.decode().rstrip('/x0')


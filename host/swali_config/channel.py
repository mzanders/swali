import struct
import asyncio


class Channel:
    async def show(self):
        reg_data = await self.node.read_reg(self.index, 0, self.num_registers)
        for reg, (descr, writeable) in self.reglist.items():
            if descr == 'Name':
                val = self._get_name(reg_data)
            else:
                val = int(reg_data[reg])
            print(' {:2} - {:18}{}: {}'.format(reg, descr, ' ' if writeable else 'R', val))

    async def menu(self):
        while True:
            await self.show()
            ui = input('Select a register to change or q to quit. > ')
            if ui == 'q':
                break
            reg = int(ui)

            try:
                if self.reglist[reg][0] == 'Name':
                    ui = input('New name? >')
                    await self.set_name(reg, ui)
                else:
                    val = struct.pack('B', int(input('New value? >')))
                    await self.node.write_reg(self.index, reg, val)
            except (KeyError, IndexError):
                print('Wrong input, try again!')
            await asyncio.sleep(0.1)

    async def set_name(self, reg, name):
        name = name.encode('UTF-8')
        name = bytearray(name) + b'\00' * (16 - len(name))
        for i in range(0, 16, 4):
            await self.node.write_reg(self.index, reg + i, name[i:i + 4])

class Light(Channel):
    def __init__(self, node, index):
        self.node = node
        self.index = index
        self.reglist = {0x03: ('Enable', True),
                        0x05: ('State', True),
                        0x06: ('Zone', True),
                        0x07: ('Subzone', True),
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

    async def enabled(self):
        return await self.node.read_reg(self.index, 0x03, 0x01) != b'\00'

    async def get_zone_subzone(self):
        reg_data = await self.node.read_reg(self.index, 0x06, 0x02)
        return int(reg_data[0]), int(reg_data[1])



class Switch(Channel):
    def __init__(self, node, index):
        self.node = node
        self.index = index
        self.reglist = {0x03: ('Enable', True),
                        0x04: ('State', False),
                        0x05: ('Class ID', True),
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

    async def quick_set(self, zone, subzone, name):
        write = True

        if await self.node.read_reg(self.index, 0x03, 1) != b'\x00':
            print('Switch channel already configured:')
            await self.show()
            text = input('Are you sure you want to overwrite these values (y/Y/yes to confirm)? > ')
            if text not in ['y', 'yes', 'Y']:
                write = False
                print('OK, not writing!')

        if write:
            await self.node.write_reg(self.index, 0x03, b'\01')  # enable
            await self.node.write_reg(self.index, 0x20, struct.pack('B', zone))
            await self.node.write_reg(self.index, 0x21, struct.pack('B', subzone))
            await self.set_name(0x10, name)


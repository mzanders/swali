import asyncio
from .vscp.util import scan_nodes, read_reg
from .vscp.tcp import TCP
from .vscp.filter import Filter
from .vscp.event import Event
from .channel import channel_reg
from .node import Node

class Gateway(TCP):
    """This class connects to the SWALI VSCP Gateway."""
    def __init__(self, *args, **kwargs):
        """Initialize a Gateway object"""
        super().__init__(*args, **kwargs)

        self.node = dict() # list of nodes
        self.ch = dict() # list of channels for each channel class
        self.ev = dict() # event sensitivity list

        for channel_type in channel_reg:
            self.ch[channel_type] = dict()
            for event_type in channel_reg[channel_type].events():
                if event_type not in self.ev:
                    self.ev[event_type] = list()
                self.ev[event_type].append(channel_reg[channel_type])

    async def _process_event(self, event):
        event_type = (event.vscp_class, event.vscp_type)
        if event_type in self.ev:
            for cls in self.ev[event_type]:
                await cls.handle_event(self.ch[cls.identifier()], event)

    async def start_update(self):
        await self.quitloop()
        flt = Filter(0,0,0,0,0,0)
        await self.setmask(flt)
        await self.setfilter(flt)
        await self.clrall()
        await self.rcvloop(self._process_event)

    async def scan(self):
        """Scan a gateway for SWALI devices, build a channel list"""
        print('scanning for VSCP CAN nodes...')
        nodes = await scan_nodes(self)
        nicknames = [nick for nick, node in nodes.items()
                     if node['stddev']==b'SWALI\0\0\0']

        for nickname in nicknames:
            self.node[nickname] = Node(self, **nodes[nickname])
            print('Nickname: {} - UID: {} - Channels: {}'.format(nickname, nodes[nickname]['uid'], nodes[nickname]['pages']))

            for channel in range(nodes[nickname]['pages']):
                channel_type = (await read_reg(self, nickname, channel, 0, num=2)).decode("utf-8")
                if channel_type in channel_reg:
                    self.ch[channel_type][(nickname, channel)] = \
                        await channel_reg[channel_type].new(self, nickname, channel)

    def get_channels(self, identifier):
        print(self.ch)
        return [obj for loc, obj in self.ch[identifier].items()]



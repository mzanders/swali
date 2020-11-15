from .vscp.util import who_is_there, read_reg, read_std_reg
from .vscp.tcp import TCP
from .vscp.filter import Filter
from .node import Node
from .channel import channel_reg

class Gateway(TCP):
    """This class connects to the SWALI VSCP Gateway."""
    def __init__(self, *args, **kwargs):
        """Initialize a Gateway object"""
        super().__init__(*args, **kwargs)

        self.node = dict() # list of nodes
        self.ch = dict() # list of channels for each channel class
        self.ev = dict() # event sensitivity list, key = event type, value = list of classes for this type
        self.groups = dict()

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
        """Scan a gateway for SWALI devices, build the channel lists"""
        await self.quitloop()
        nodes = dict()

        for nickname in range(128):
            (guid, mdf) = await who_is_there(self, nickname)

            if (guid, mdf) != (None, None):
                node = await Node.new(self, nickname, guid, mdf)
                nodes[nickname] = node
                if node.is_swali:
                    for channel_type in channel_reg:
                        self.ch[channel_type].update(node.channels[channel_type])

        self.update_groups()

    def get_channels(self, identifier):
        return [obj for loc, obj in self.ch[identifier].items()]

    def update_groups(self):
        self.groups = dict()
        for channel_type in channel_reg:
            for (nick, ch_nr), channel in self.ch[channel_type].items():
                if hasattr(channel, 'zone') and hasattr(channel, 'subzone'):
                    group_id = (channel.zone, channel.subzone)
                    if group_id != (0, 0):
                        if group_id in self.groups:
                            self.groups[group_id].add(channel)
                        else:
                            self.groups[group_id] = {channel}

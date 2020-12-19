from vscp.util import who_is_there
from vscp.tcp import TCP
from vscp.filter import Filter
from node import Node


class Gateway(TCP):
    """This class connects to the SWALI VSCP Gateway."""
    def __init__(self, *args, **kwargs):
        """Initialize a Gateway object"""
        super().__init__(*args, **kwargs)
        self.nodes = dict() # list of nodes

    async def start_update(self):
        await self.quitloop()
        flt = Filter(0,0,0,0,0,0)
        await self.setmask(flt)
        await self.setfilter(flt)
        await self.clrall()
        await self.rcvloop(self._process_event)

    async def scan(self):
        """Scan a gateway for devices, build the channel lists"""
        await self.quitloop()

        for nickname in range(128):
            (guid, mdf) = await who_is_there(self, nickname)

            if (guid, mdf) != (None, None):
                try:
                    device_obj = __import__(mdf)
                    self.nodes[nickname] = await getattr(device_obj, mdf.title()).new(self, nickname, guid, mdf)
                except (ImportError, ValueError):
                    self.nodes[nickname] = Node(self, nickname)
                    await self.nodes[nickname].init()

                #node = await Node.new(self, nickname, guid, mdf)
                #self.nodes[nickname] = node

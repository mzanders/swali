from node import Node
from channel import Switch


class Beijing_Z01(Node):
    def __init__(self, gw, nick, guid, mdf):
        super().__init__(gw, nick, guid, mdf)
        for i in range(10):
            self.channels.append(Switch(self, i))

    @classmethod
    async def new(cls, gw, nick, guid, mdf):
        node = cls(gw, nick, guid, mdf)
        await node.init()
        return node

from node import Node
from channel import Light, Switch


class Paris_Z01(Node):
    def __init__(self, gw, nick, guid, mdf):
        super().__init__(gw, nick, guid, mdf)
        for i in range(7):
            self.channels.append(Light(self, i))

    @classmethod
    async def new(cls, gw, nick, guid, mdf):
        node = cls(gw, nick, guid, mdf)
        await node.init()
        return node

from node import Node
from channel import Light, Switch


class Paris_Z01(Node):
    def __init__(self, gw, nick):
        super().__init__(gw, nick)
        self.channels = list()
        for i in range(7):
            self.channels.append(Light(self, i))

    @classmethod
    async def new(cls, gw, nick):
        node = cls(gw, nick)
        await node.init()
        return node

from .vscp.util import write_reg


class Node:
    def __init__(self, bus, nickname=None, guid=None, uid=None, pages=None, **kwargs):
        self.bus = bus
        self.nickname = nickname
        self.guid = guid
        self.uid = uid
        self.pages = pages

    async def set_uid(self, uid):
        if not isinstance(uid, (bytes, bytearray)) or (len(uid) != 5):
            raise ValueError('Incorrect UID')

        for i, value in enumerate(uid):
            await write_reg(self.bus, 0, 0x84+i, self.nickname, value)
        self.uid = bytes(uid)
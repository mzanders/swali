import asyncio
from .tcp import TCP
from .const import (CLASS_VSCP,
                    EVENT_WHO_IS_THERE,
                    EVENT_WHO_IS_THERE_RESPONSE,
                    EVENT_EXT_PAGE_RESP,
                    EVENT_EXT_PAGE_READ,
                    EVENT_EXT_PAGE_WRITE)
from .filter import Filter
from .event import Event
from .guid import Guid
import struct
import time

async def write_reg(vscp, page, reg, nickname, value):
    if(len(value) > 4):
        raise ValueError('Register write limited to 4 bytes')
    data_prefix = struct.pack('>BHB', nickname, page, reg)
    await vscp.send(event(vscp_class = CLASS_VSCP,
                          vscp_type = EVENT_EXT_PAGE_WRITE,
                          data = data_prefix + value))

async def read_reg(vscp, nickname, page, reg, num=1):
    if num == 0:
        return bytearray()
    if num == 256:
        num_cmd = 0
    else:
        num_cmd = num

    await vscp.quitloop()
    flt = Filter(0,0,0,0x3ff,EVENT_EXT_PAGE_RESP,0xFF)
    await vscp.setmask(flt)
    await vscp.setfilter(flt)
    await vscp.clrall()

    tx_event = Event(vscp_class = CLASS_VSCP,
                     vscp_type  = EVENT_EXT_PAGE_READ,
                     data = struct.pack('>BHBB', nickname, page, reg,num_cmd))

    await vscp.send(tx_event)

    await asyncio.sleep(0.001 * num + 0.01)
    resp = await vscp.retr(int(num/4)+2)
    resp = [x for x in resp[1] if x.guid.nickname == nickname]
    result=bytearray()
    for item in resp:
        result[item.data[3]:item.data[3]+len(item.data)-4] = item.data[4:]
    return result


async def scan_nodes(vscp):
    await vscp.quitloop()
    flt = Filter(0,0,0,0x3ff,EVENT_WHO_IS_THERE_RESPONSE,0xFF)
    await vscp.setmask(flt)
    await vscp.setfilter(flt)
    await vscp.clrall()

    nodes = dict()

    for node in range(128):

        await vscp.send(Event(vscp_class = 0, vscp_type=EVENT_WHO_IS_THERE,
                              data=struct.pack('>B', node)))
        await asyncio.sleep(0.01) # allow time for replies & spare bandwidth
        resp = await vscp.retr(100)
        filtered = [x for x in resp[1] if x.guid.nickname == node]
        if len(filtered) == 7:
            raw = bytearray(7*7)
            for ev in filtered:
                offset = int(ev.data[0]) * 7
                raw[offset:offset+7] = ev.data[1:]
            nodes[node] = dict()
            nodes[node]['guid'] = Guid(input=raw[0:16])
            nodes[node]['mdf'] = raw[16:].split(b'\0')[0].decode()


    for nickname, descr in nodes.items():
        descr['pages'] = \
            int((await read_reg(vscp, nickname, 0, 0x99, num=1))[0])
        descr['stddev'] = await read_reg(vscp, nickname, 0, 0x9A, num=8)
        descr['uid'] = bytes(await read_reg(vscp, nickname, 0, 0x84, num=5))

    return nodes

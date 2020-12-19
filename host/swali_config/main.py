import asyncio
from gateway import Gateway
from vscp.const import EVENT_INFORMATION_ON, CLASS_INFORMATION
from vscp.filter import Filter


class NoInput(Exception):
    pass

def show_nodes(gw):
    print('Node list')
    for nick, node in gw.nodes.items():
        print('{:3} - {:14} - {}'.format(nick, type(node).__name__, node.version))

async def wait_onoff_event(vscp):
    await vscp.quitloop()
    flt = Filter(0,0,CLASS_INFORMATION,0x3ff,EVENT_INFORMATION_ON,0xFF)
    await vscp.setmask(flt)
    await vscp.setfilter(flt)
    await vscp.clrall()

    for i in range(30,0,-1):
        await asyncio.sleep(0.5)
        print(i)
        resp = (await vscp.retr(1))
        if bytes('+OK'.encode()) in resp[0]:
            break
        else:
            resp = None
    if resp is None:
        raise NoInput
    ev = resp[1][0]
    return ev.guid.nickname, ev.data[0]

async def binding_mode(gw):
    lights = list()
    for node in gw.nodes.values():
        lights.extend([ch for ch in node.channels if (type(ch).__name__ == 'Light') and await ch.enabled()])

    while(True):
        names = [await light.name() for light in lights]
        for idx, light in enumerate(lights):
            print('{} - {}'.format(idx, names[idx]))
        id = input('Select light to bind to, q to quit. > ')
        if id == 'q':
            break
        try:
            id = int(id)
            zone, subzone = await lights[id].get_zone_subzone()
            print('Push the light switch now.')
            nick, channel = await wait_onoff_event(gw)
            await gw.nodes[nick].channels[channel].quick_set(zone, subzone, names[id])
            print('Done!')
        except IndexError:
            print('Invalid entry!')
        except NoInput:
            print('Did not get an input!')
        except AttributeError:
            print('On/off event didn\'t come from switch?')



async def main():
    gw = Gateway()
    await gw.connect()
    await gw.scan()
    cont = True

    while(cont):
        show_nodes(gw)
        node = input('Select node? Enter q to quit, b to enter binding mode.\n> ')

        if node == 'q':
            cont = False
            print('Quitting... bye!')
        if node == 'b':
            await binding_mode(gw)
        else:
            try:
                await gw.nodes[int(node)].menu()
            except (IndexError, ValueError):
                print('Invalid node selected, try again.')

if __name__== "__main__":
    asyncio.run(main())
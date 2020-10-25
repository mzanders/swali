import asyncio
from .gateway import Gateway
from .light import Light
from .switch import Switch

async def callback_light(light, state):
    print('Light {} >>> {}'.format(light.name, state))

async def callback_switch(sw, state):
    print('Button {} >>> {}'.format((sw.nickname, sw.channel), state))

async def main():
    gw = Gateway()
    await gw.connect()
    await gw.scan()
    lights = [x for x in gw.get_channels(Light.identifier()) if x.enabled]
    print('lights:')
    for light in lights:
        print('  {:<16} - {}'.format(light.name, light.state))
        light.add_callback(callback_light)

    switches = gw.get_channels(Switch.identifier())
    for sw in switches:
        sw.add_callback(callback_switch)

    await gw.start_update()

    await asyncio.sleep(20.0)

    await gw.close()

if __name__ == "__main__":
    import sys
    asyncio.run(main())

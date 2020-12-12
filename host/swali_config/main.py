import asyncio
from gateway import Gateway


def show_nodes(gw):
    print('Node list')
    for nick, node in gw.nodes.items():
        print('{:3} - {}'.format(nick, type(node).__name__))


async def main():
    gw = Gateway()
    await gw.connect()
    await gw.scan()
    cont = True

    while(cont):
        show_nodes(gw)
        node = input('Select node? Enter q to quit.\n> ')

        if node == 'q':
            cont = False
            print('Quitting... bye!')
        else:
            try:
                await gw.nodes[int(node)].menu()
            except (IndexError, ValueError):
                print('Invalid node selected, try again.')

if __name__== "__main__":
    asyncio.run(main())
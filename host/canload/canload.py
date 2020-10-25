import intelhex
import can
import time
import enum
import argparse
import struct

pb_length = 40

class msg(enum.Enum):
    put_cntrl = 0x1000
    put_data  = 0x1100
    get_cntrl = 0x1200
    get_data  = 0x1300
    
class cmd(enum.Enum):
    nop = 0x00             # Do nothing
    reset = 0x01           # Issue a soft reset
    rst_chksm = 0x02       # Reset the checksum counter and verify status
    chk_run = 0x03         # Add checksum to special data, if verify and zero checksum
                           # then clear first location of EEDATA.
           
# Print iterations progress, from https://stackoverflow.com/questions/3173320/text-progress-bar-in-the-console
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()           
           
           
class no_reply(Exception):
    pass
           
def auto_int(x):
    return int(x, 0)
           
def segment(x):
    if x < 0x200000:
        return 'prog mem'
    if x < 0x300000:
        return 'user id'
    if x < 0x3FFFF8:
        return 'config'
    if x == 0x3FFFF8:
        return 'device id'
    if x >= 0xf00000:
        return 'eedata'
           
def send_msg(bus, msg_type, data=bytearray()):
    tx_msg = can.Message(arbitration_id=msg_type.value, data=data, is_extended_id=True)
    bus.send(tx_msg)

    while True:
        rx_msg = bus.recv(0.5)
        if rx_msg == None:            
            raise no_reply
        else:
            # the set filter ensures we're getting data from the right node
            if (rx_msg.arbitration_id & 0x0100) == (msg_type.value & 0x0100):
                return rx_msg.data                
                

def control_reg(address=0x000000, write=False, cmd=cmd.nop, sp_data=0x0000):
    def_flags = 0x1C #auto erase, auto inc, ack
    write_flag = 0x01
    rv = bytearray(8)
    rv[0] = address & 0xff
    rv[1] = (address & 0x00ff00) >> 8
    rv[2] = (address & 0xff0000) >> 16
    rv[3] = 0x00
    rv[4] = def_flags
    if write:
        rv[4] |= write_flag
    rv[5] = cmd.value & 0xff
    rv[6] = sp_data & 0xff
    rv[7] = (sp_data & 0xff00) >> 8
    return rv

def iter_hex(bus, ih, verify=False, progress=True):
    data_buffer = bytearray(8)
    checksum = 0    
    
    for start, end in ih.segments():
        if segment(start) == 'config': # skip config space! don't overwrite values set by bootloader
            continue
        start = (start//8)*8
        end = ((end + 7)//8)*8
        segstring = '   segment {0:12}: 0x{1:06X}-0x{2:06X}'.format(segment(start), start, end)
        if not progress:
            print(segstring)
        
        send_msg(bus, msg.put_cntrl, data=control_reg(address=start, write=not(verify)))
        
        for i in range(start, end):            
            data = ih[i]
            checksum += data
            data_buffer[i%8] = data
            if i%8 == 7:
                if progress:
                    printProgressBar(i-start, end-start-1, prefix=segstring, length=pb_length)
                if(verify):
                    rb = send_msg(bus, msg.get_data)
                    if rb != data_buffer:
                        print('Error during verification at 0x{:06X}'.format(i))
                        exit(0)
                else:
                    send_msg(bus, msg.put_data, data_buffer)
            
           
    return checksum
    
def eedata(bus, size, verify=False, progress=True):
    data_buffer = bytearray(b'\xFF')*8
    checksum = 0
    
    start = 0xf00000
    end = 0xf00000 + size
    end = ((end + 7)//8)*8 # round to next multiple of 8    
    segstring = '   segment {0:12}: 0x{1:06X}-0x{2:06X}'.format(segment(start), start, end)
    if not progress:
            print(segstring)
    
    send_msg(bus, msg.put_cntrl, data=control_reg(address=start, write=not(verify)))
            
    for i in range(start, end):
        checksum += 0xFF        
        if i%8 == 7:
            if progress:
                printProgressBar(i-start, end-start-1, prefix=segstring, length=pb_length)
            if verify:
                rb = send_msg(bus, msg.get_data)
                if rb != data_buffer:
                    print('Error during EEPROM verification at 0x{:06X}'.format(i))
                    exit(0)                
            else:
                send_msg(bus, msg.put_data, data_buffer)     
            
    return checksum

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Program a PIC controller on CAN bus using the VSCP CAN Bootloader. Please look at Python-Can documentation for CAN configuration.")
    parser.add_argument('filename', help='Intel HEX file to be programmed')
    parser.add_argument('-c', '--context', dest='context', default='default', help='CAN bus context to use from CAN config file')
    parser.add_argument('-n', '--nick', dest='nickname', help='VSCP Nickname of node (default=0xFE)', default=0xFE, type=auto_int)
    parser.add_argument('-P', '--noprogress', dest='progress', action='store_false', help='No progress indicators')
    parser.add_argument('-V', '--noverify', dest='verify', action='store_false', help='No verification after writing')
    parser.add_argument('-R', '--noreset', dest='reset', action='store_false', help='No reset when done')
    parser.add_argument('-e', '--eedataerase', dest='ereedata', action='store_true', help='Erase EEDATA')
    parser.add_argument('-s', '--eedatasize', dest='eedatasize', help='EEDATA size for erasing (default=0xFF)', default=0xFF, type=auto_int)
        
    args = parser.parse_args()
    
    ih = intelhex.IntelHex(args.filename)  # this performs checks on the input file
    bus = can.interface.Bus(context=args.context)
    
    filters = [{"can_id":   0x00001400 | args.nickname, 
                "can_mask": 0x1FFFFEFF,
                "extended": True}]
    bus.set_filters(filters)
    
    try:
        send_msg(bus, msg.put_cntrl, data=control_reg(cmd=cmd.rst_chksm))
    except no_reply:
        print('Did not get a reply, correct nickname?')
        exit()
     
    print('Program...')
    checksum = iter_hex(bus, ih, progress=args.progress)
    
    if(args.ereedata):
        checksum += eedata(bus, args.eedatasize, progress=args.progress)
        
    checksum = (0xFFFF-(checksum & 0xFFFF)+1) & 0xFFFF
    
    if(args.verify):
        print('Verify...')
        iter_hex(bus, ih, verify=True, progress=args.progress)
        if(args.ereedata):
            eedata(bus, args.eedatasize, progress=args.progress, verify=True)
    
    print('Perform checksum check in target...')
    send_msg(bus, msg.put_cntrl, data=control_reg(cmd=cmd.chk_run, sp_data=checksum, write = True))
    
    if(args.reset):
        print('Resetting device...')
        send_msg(bus, msg.put_cntrl, data=control_reg(cmd=cmd.reset))
    
    
            
    
    


DEF_HOST = '127.0.0.1'
DEF_PORT = 8598
DEF_USER = None
DEF_PASSWORD = None

CLASS_VSCP = 0
CLASS_INFORMATION = 0x14
CLASS_CONTROL = 0x1E

EVENT_WHO_IS_THERE = 0x1F
EVENT_WHO_IS_THERE_RESPONSE = 0x20
EVENT_EXT_PAGE_READ = 0x25
EVENT_EXT_PAGE_WRITE = 0x26
EVENT_EXT_PAGE_RESP = 0x27

EVENT_INFORMATION_BUTTON = 0x01
EVENT_INFORMATION_ON = 0x03
EVENT_INFORMATION_OFF = 0x04

EVENT_CONTROL_TURN_ON = 0x05
EVENT_CONTROL_TURN_OFF = 0x06

STD_REG_UID = 0x84
STD_REG_PAGES = 0x99
STD_REG_STD_DEV = 0x9A

STD_REG_LENGTH = {STD_REG_STD_DEV : 8,
                  STD_REG_PAGES : 1,
                  STD_REG_UID : 5}
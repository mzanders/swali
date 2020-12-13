# SWALI - VSCP firmware for switches and lights

https://www.vscp.org
https://www.home-assistant.io

## Scope
The firmwares presented in this project are alternatives to the default
firmware by the vendor of the Paris and Beijing modules.

This firmware follows VSCP4HASS, https://www.github.com/mzanders/vscp4hass.

## Projects
SWALI consists of these projects:
   - prj/bootloader_PIC18F2580: CAN bootloader
   - prj/beijing: 10 switch input module
   - prj/paris: 7 light output module
   - host/canload: Python firmware loader, using python-CAN
   - host/swali_config: Python script to configure the modules, using a remote
     connection to uvscpd/vscpd.

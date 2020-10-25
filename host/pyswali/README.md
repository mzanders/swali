Python package to communicate with the [SWALI VSCP Framework](http://github.com/mzanders/swali) By using this library you can communicate with the gateway (vscpd or uvscpd) and control lights and buttons which conform to the SWALI framework on VSCP.

Some of the features include:

- Get information on the gateway
- Observe lights, groups and other resources and get notified when they change
- List all devices connected to gateway
- List all lights and get attributes of lights (name, state etc)
- Change attribute values of lights (state, dimmer level etc)
- Observe buttons/switches

Table of contents:

1. [Installation](#installation)
2. [Stand-alone use (command-line interface)](#stand-alone-use-command-line-interface)
3. [Implement in your own Python platform](#implement-in-your-own-python-platform)
4. [Acknowledgements](#acknowledgements)

## Installation
You might have to use superuser privileges (sudo) for some commands to work when installing.

This library targets asynchronous applications only. Dateutil is required as external dependency. Communication with the gateway is done using a TCP socket.

Note that the password is currently sent as bare text across the network. It is advised to keep this interface on the loopback port until/if SSL is supported.

## Stand-alone use (command-line interface)

If you want to test this library stand-alone in a command-line interface:

```shell
$ python3 -i -m pyswali --host=IP --port=PORT --user=USER --password=PASSWORD
```
Where **IP**, **PORT**, **USER** and **PASSWORD** are substituted by the required values for the vscpd/uvscpd instance.
Default values are:
- IP: 127.0.0.1
- Port: 8598
- User: None
- Password: None

When either user/password is None, no authentication is performed after connecting to the server.

## Implement in your own Python platform

Please see the example files. TO DO

## Acknowledgements

This library is using [VSCP](https://www.vscp.org) by [Akhe Hedman](http://github.com/grodansparadis).

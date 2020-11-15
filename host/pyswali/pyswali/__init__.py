"""Implement an API wrapper around SWALI VSCP."""

from .error import (PyswaliError, RequestError, RequestTimeout)
from .gateway import Gateway
from .light import Light
from .switch import Switch

__all__ = ['Gateway', 'Light', 'Switch',
           'PyswaliError', 'RequestError', 'RequestTimeout']

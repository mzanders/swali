"""Errors for pyswali"""

class PyswaliError(Exception):
    """Base Error"""
    pass

class RequestError(PyswaliError):
    """An error happened sending or receiving a command."""
    pass

class RequestTimeout(RequestError):
    """Error when sending or receiving the command timed out."""
    pass

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import setup, find_packages

# To use a consistent encoding
from codecs import open
from os import path

here = path.abspath(path.dirname(__file__))

# Get the long description from the README file
with open(path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

VERSION = '0.0.1'
DOWNLOAD_URL = \
    'https://github.com/mzanders/pyswali/archive/{}.zip'.format(VERSION)

setup(
  name='pyswali',
  #packages=PACKAGES,
  python_requires='>=3.5',
  version=VERSION,
  description='SWALI VSCP control your buttons and lights from python',
  long_description=long_description,
  author='mzanders',
  author_email='no@email.com',
  long_description_content_type="text/markdown",
  url='https://github.com/mzanders/pyswali',
  license='MIT',
  keywords='vscp vscpd uvscpd swali can canbus can-bus iot light homeautomation',
  #download_url=DOWNLOAD_URL,
  #extras_require=EXTRAS_REQUIRE,
)

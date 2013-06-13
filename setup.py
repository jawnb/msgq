#!/usr/bin/env python
#
# Setup script for msgq

from setuptools import setup, Extension, find_packages
print find_packages()
setup(name='msgq',
      version='0.1',
      description='System V IPC Message Queue Python Extension Module',
      author='Lars Djerf',
      author_email='lars.djerf@gmail.com',
      url='',
      ext_modules=[Extension('_msgq', ['_msgq.c'])],
      packages=find_packages(),
      test_suite="tests"
      )

# python extension setup script for urjtag

from distutils.core import setup, Extension
setup(name="urjtag",
      version="1.0",
      description="urJtag Python Bindings",
      ext_modules=[
        Extension("urjtag", ["chain.c"],
                  define_macros=[('HAVE_CONFIG_H', None)],
                  include_dirs=['../..', '../../include'],
                  library_dirs=['../../src/.libs'],
                  libraries=['urjtag'])
         ])

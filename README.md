libdcp
======

Hello.


Acknowledgements
================

Wolfgang Woehl's cinemaslides was most informative on the
nasty details of encryption.

libdcp is written by Carl Hetherington and Mart Jansink


Building
========

    ./waf configure
    ./waf
    sudo ./waf install


Dependencies
============

boost filesystem, signals2 and unit testing libraries
openssl
libsigc++
libxml++
xmlsec
openjpeg (1.5.0 or above)


Documentation
=============

Run doxygen in the top-level directory and then see build/doc/html/index.html.

There are some examples in the examples/ directory.

# libdcp

Library for reading and writing Digital Cinema Packages (DCPs).


# Acknowledgements

Wolfgang Woehl's cinemaslides was most informative on the
nasty details of encryption.

libdcp is written by Carl Hetherington and Mart Jansink.
Bugfixes were received from Philip Tschiemer.


# Building

```
  ./waf configure
  ./waf
  sudo ./waf install
```


# Dependencies

- pkg-config (for build system)
- boost (1.45 or above): filesystem, signals2, datetime and unit testing libraries
- openssl
- libsigc++
- libxml++
- xmlsec
- ImageMagick or GraphicsMagick
- sndfile
- openjpeg (1.5.0 or above)
- [libasdcp-cth](https://github.com/cth103/asdcplib-cth/tree/cth)
- [libcxml](https://github.com/cth103/libcxml)
- (optional) OpenMP
- (optional) gcov (for tests)


# Build options

```
  --target-windows      set up to do a cross-compile to Windows
  --enable-debug        build with debugging information and without optimisation
  --static              build libdcp statically, and link statically to openjpeg, cxml, asdcplib-cth
  --disable-tests       disable building of tests
  --disable-gcov        dont use gcov in tests
  --disable-examples    disable building of examples
  --enable-openmp       enable use of OpenMP
  --openmp=OPENMP       Specify OpenMP Library to use: omp, gomp (default), iomp..
  --jpeg=JPEG           specify JPEG library to build with: oj1 or oj2 for OpenJPEG 1.5.x or OpenJPEG 2.1.x respectively
  --force-cpp11         force use of C++11
```

# A note on building for macOS

As goto solution, all dependencies can be installed using [Homebrew](https://brew.sh/).
Make sure to add the respective `PKG_CONFIG_PATH` paths so the packages are indeed found.

```bash
## Default Homebrew paths
PKG_CONFIG_PATH="/usr/local/opt/icu4c/lib/pkgconfig"
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/opt/pangomm/lib/pkgconfig"
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/opt/libffi/lib/pkgconfig" # needed by gobject2
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/opt/openssl/lib/pkgconfig"
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/opt/libxml2/lib/pkgconfig"

## Set as installed
# PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/path-to-your-install-folder/libasdcp-cth"
# PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/path-to-your-install-folder/libcxml"

export PKG_CONFIG_PATH
```

If you want support for *OpenMP*, the standard llvm compiler coming with Xcode (or rather its command line tools) does not support it such that you will have to override the compiler (using the `CXX` environment variable).
The version provided through Homebrew will work (or any of your choice) along with `--openmp=omp`.

```bash
## Default Homebrew path
export CXX=/usr/local/opt/llvm/bin/clang++
```


# Documentation

Run doxygen in the top-level directory and then see build/doc/html/index.html.

There are some examples in the examples/ directory.


# Coding style

* Use C++11 but nothing higher, as we need the library to be usable on some quite old compilers.
* Put a Doxygen @file comment under the GPL banner in each source file.
* Two blank lines between methods, and between 'blocks' in headers.
* Doxygen comments in header files for public methods, source files for protected / private methods; no full stops after simple doxygen strings.
* Use `= delete` on copy constructors and assignment operators instead of boost::noncopyable.
* Initialise POD members in classes in the header.
* Use std::make_shared to create shared pointers to things.

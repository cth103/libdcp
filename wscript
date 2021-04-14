#
#    Copyright (C) 2012-2020 Carl Hetherington <cth@carlh.net>
#
#    This file is part of libdcp.
#
#    libdcp is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    libdcp is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.
#
#    In addition, as a special exception, the copyright holders give
#    permission to link the code of portions of this program with the
#    OpenSSL library under certain conditions as described in each
#    individual source file, and distribute linked combinations
#    including the two.
#
#    You must obey the GNU General Public License in all respects
#    for all of the code used other than OpenSSL.  If you modify
#    file(s) with this exception, you may extend this exception to your
#    version of the file(s), but you are not obligated to do so.  If you
#    do not wish to do so, delete this exception statement from your
#    version.  If you delete this exception statement from all source
#    files in the program, then also delete it here.
#

import subprocess
import os
import sys
import shlex
import distutils.spawn
from waflib import Logs, Context

APPNAME = 'libdcp'

this_version = subprocess.Popen(shlex.split('git tag -l --points-at HEAD'), stdout=subprocess.PIPE).communicate()[0]
last_version = subprocess.Popen(shlex.split('git describe --tags --abbrev=0'), stdout=subprocess.PIPE).communicate()[0]

if this_version == '':
    VERSION = '%sdevel' % last_version[1:].strip()
else:
    VERSION = this_version[1:].strip()

API_VERSION = '-1.0'

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--target-windows', action='store_true', default=False, help='set up to do a cross-compile to Windows')
    opt.add_option('--enable-debug', action='store_true', default=False, help='build with debugging information and without optimisation')
    opt.add_option('--static', action='store_true', default=False, help='build libdcp statically, and link statically to openjpeg, cxml, asdcplib-carl')
    opt.add_option('--disable-tests', action='store_true', default=False, help='disable building of tests')
    opt.add_option('--disable-benchmarks', action='store_true', default=False, help='disable building of benchmarks')
    opt.add_option('--disable-gcov', action='store_true', default=False, help='don''t use gcov in tests')
    opt.add_option('--disable-examples', action='store_true', default=False, help='disable building of examples')
    opt.add_option('--enable-openmp', action='store_true', default=False, help='enable use of OpenMP')
    opt.add_option('--openmp', default='gomp', help='specify OpenMP Library to use: omp, gomp (default), iomp')
    opt.add_option('--jpeg', default='oj2', help='specify JPEG library to build with: oj1 or oj2 for OpenJPEG 1.5.x or OpenJPEG 2.1.x respectively')

def configure(conf):
    conf.load('compiler_cxx')
    conf.load('clang_compilation_database', tooldir=['waf-tools'])
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Wextra', '-D_FILE_OFFSET_BITS=64', '-D__STDC_FORMAT_MACROS', '-std=c++11'])
    gcc = conf.env['CC_VERSION']
    if int(gcc[0]) >= 4 and int(gcc[1]) > 1:
        conf.env.append_value('CXXFLAGS', ['-Wno-maybe-uninitialized'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    conf.env.TARGET_WINDOWS = conf.options.target_windows
    conf.env.TARGET_OSX = sys.platform == 'darwin'
    conf.env.TARGET_LINUX = not conf.env.TARGET_WINDOWS and not conf.env.TARGET_OSX
    conf.env.ENABLE_DEBUG = conf.options.enable_debug
    conf.env.DISABLE_TESTS = conf.options.disable_tests
    conf.env.DISABLE_BENCHMARKS = conf.options.disable_benchmarks
    conf.env.DISABLE_EXAMPLES = conf.options.disable_examples
    conf.env.STATIC = conf.options.static
    conf.env.API_VERSION = API_VERSION

    if conf.env.TARGET_WINDOWS:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_WINDOWS')
    if conf.env.TARGET_OSX:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_OSX')
    if conf.env.TARGET_LINUX:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_LINUX')

    if conf.env.TARGET_OSX:
        conf.env.append_value('CXXFLAGS', ['-Wno-unused-result', '-Wno-unused-parameter', '-Wno-unused-local-typedef'])
        conf.env.append_value('LINKFLAGS', '-headerpad_max_install_names')
    else:
        conf.env.append_value('CXXFLAGS', ['-Wsuggest-override'])

    # Disable libxml++ deprecation warnings for now
    conf.env.append_value('CXXFLAGS', ['-Wno-deprecated-declarations'])

    if conf.options.enable_openmp:
        conf.env.append_value('CXXFLAGS', ['-fopenmp', '-DLIBDCP_OPENMP'])
        conf.env.LIB_OPENMP = [conf.options.openmp]
        conf.env.append_value('LDFLAGS', ['-l%s' % conf.options.openmp])
        conf.check_cxx(cxxflags='-fopenmp', msg='Checking that compiler supports -fopenmp')

    if not conf.env.TARGET_WINDOWS:
        conf.env.append_value('LINKFLAGS', '-pthread')

    if conf.env.TARGET_LINUX:
        conf.check(lib='dl', uselib_store='DL', msg='Checking for library dl')

    if conf.options.jpeg == 'oj1':
        conf.env.append_value('CXXFLAGS', ['-DLIBDCP_OPENJPEG1'])
    elif conf.options.jpeg == 'oj2':
        conf.env.append_value('CXXFLAGS', ['-DLIBDCP_OPENJPEG2'])
    else:
        Logs.error('Invalid --jpeg value %s' % conf.options.jpeg)

    conf.check_cfg(package='openssl', args='--cflags --libs', uselib_store='OPENSSL', mandatory=True)
    conf.check_cfg(package='libxml++-2.6', args='--cflags --libs', uselib_store='LIBXML++', mandatory=True)
    conf.check_cfg(package='xmlsec1', args='--cflags --libs', uselib_store='XMLSEC1', mandatory=True)
    # Remove erroneous escaping of quotes from xmlsec1 defines
    conf.env.DEFINES_XMLSEC1 = [f.replace('\\', '') for f in conf.env.DEFINES_XMLSEC1]

    # ImageMagick / GraphicsMagick
    if distutils.spawn.find_executable('Magick++-config'):
        conf.check_cfg(package='', path='Magick++-config', args='--cppflags --cxxflags --libs', uselib_store='MAGICK', mandatory=True, msg='Checking for ImageMagick/GraphicsMagick')
    else:
        image = conf.check_cfg(package='ImageMagick++', args='--cflags --libs', uselib_store='MAGICK', mandatory=False)
        graphics = conf.check_cfg(package='GraphicsMagick++', args='--cflags --libs', uselib_store='MAGICK', mandatory=False)
        if image is None and graphics is None:
            Logs.error('Neither ImageMagick++ nor GraphicsMagick++ found: one or the other is required')

    conf.check_cfg(package='sndfile', args='--cflags --libs', uselib_store='SNDFILE', mandatory=False)

    if conf.options.static:
        if conf.options.jpeg == 'oj2':
            conf.check_cfg(package='libopenjp2', args='--cflags', atleast_version='2.1.0', uselib_store='OPENJPEG', mandatory=True)
            conf.env.STLIB_OPENJPEG = ['openjp2']
        elif conf.options.jpeg == 'oj1':
            conf.check_cfg(package='libopenjpeg1', args='--cflags', atleast_version='1.5.0', uselib_store='OPENJPEG', mandatory=True)
            conf.env.STLIB_OPENJPEG = ['openjpeg']
        conf.check_cfg(package='libasdcp-carl', atleast_version='0.1.3', args='--cflags', uselib_store='ASDCPLIB_CTH', mandatory=True)
        conf.env.HAVE_ASDCPLIB_CTH = 1
        conf.env.STLIB_ASDCPLIB_CTH = ['asdcp-carl', 'kumu-carl']
        conf.env.HAVE_CXML = 1
        conf.env.LIB_CXML = ['xml++-2.6', 'glibmm-2.4']
        conf.env.STLIB_CXML = ['cxml']
        conf.check_cfg(package='xerces-c', args='--cflags', uselib_store='XERCES', mandatory=True)
        conf.env.LIB_XERCES = ['xerces-c', 'icuuc', 'curl']
    else:
        if conf.options.jpeg == 'oj2':
            conf.check_cfg(package='libopenjp2', args='--cflags --libs', atleast_version='2.1.0', uselib_store='OPENJPEG', mandatory=True)
        elif conf.options.jpeg == 'oj1':
            conf.check_cfg(package='libopenjpeg1', args='--cflags --libs', atleast_version='1.5.0', uselib_store='OPENJPEG', mandatory=True)
        conf.check_cfg(package='libasdcp-carl', atleast_version='0.1.3', args='--cflags --libs', uselib_store='ASDCPLIB_CTH', mandatory=True)
        conf.check_cfg(package='libcxml', atleast_version='0.17.0', args='--cflags --libs', uselib_store='CXML', mandatory=True)
        conf.check_cfg(package='xerces-c', args='--cflags --libs', uselib_store='XERCES', mandatory=True)

    if conf.options.target_windows:
        # XXX: it feels like there should be a more elegant way to get these included
        conf.env.LIB_XERCES.append('curl')
        conf.env.LIB_XERCES.append('ws2_32')

    if conf.options.target_windows:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    if conf.options.enable_debug:
        conf.env.append_value('CXXFLAGS', '-g')
    else:
        # Somewhat experimental use of -O2 rather than -O3 to see if
        # Windows builds are any more reliable
        conf.env.append_value('CXXFLAGS', '-O2')

    # We support older boosts on Linux so we can use the distribution-provided package
    # on Centos 7, but it's good if we can use 1.61 for boost::dll::program_location()
    boost_version = ('1.45', '104500') if conf.env.TARGET_LINUX else ('1.61', '106800')

    conf.check_cxx(fragment="""
                            #include <boost/version.hpp>\n
                            #if BOOST_VERSION < %s\n
                            #error boost too old\n
                            #endif\n
                            int main(void) { return 0; }\n
                            """ % boost_version[1],
                   mandatory=True,
                   msg='Checking for boost library >= %s' % boost_version[0],
                   okmsg='yes',
                   errmsg='too old\nPlease install boost version %s or higher.' % boost_version[0])

    conf.check_cxx(fragment="""
    			    #include <boost/filesystem.hpp>\n
    			    int main() { boost::filesystem::copy_file ("a", "b"); }\n
			    """,
                   msg='Checking for boost filesystem library',
                   libpath='/usr/local/lib',
                   lib=['boost_filesystem%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store='BOOST_FILESYSTEM')

    conf.check_cxx(fragment="""
    			    #include <boost/signals2.hpp>\n
    			    int main() { boost::signals2::signal<void (int)> x; }\n
			    """,
                   msg='Checking for boost signals2 library',
                   uselib_store='BOOST_SIGNALS2')

    conf.check_cxx(fragment="""
    			    #include <boost/date_time.hpp>\n
    			    int main() { boost::gregorian::day_clock::local_day(); }\n
			    """,
                   msg='Checking for boost datetime library',
                   libpath='/usr/local/lib',
                   lib=['boost_date_time%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store='BOOST_DATETIME')

    if not conf.env.DISABLE_TESTS:
        conf.recurse('test')
        if not conf.options.disable_gcov:
            conf.check(lib='gcov', define_name='HAVE_GCOV', mandatory=False)
            conf.env.append_value('LINKFLAGS', '-fprofile-arcs')

def build(bld):
    create_version_cc(bld, VERSION)

    if bld.env.TARGET_WINDOWS:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    libs="-L${libdir} -ldcp%s -lcxml -lboost_system%s" % (bld.env.API_VERSION, boost_lib_suffix)
    if bld.env.TARGET_LINUX:
        libs += " -ldl"

    bld(source='libdcp%s.pc.in' % bld.env.API_VERSION,
        version=VERSION,
        includedir='%s/include/libdcp%s' % (bld.env.PREFIX, bld.env.API_VERSION),
        libs=libs,
        install_path='${LIBDIR}/pkgconfig')

    bld.recurse('src')
    bld.recurse('tools')
    if not bld.env.DISABLE_TESTS:
        bld.recurse('test')
    if not bld.env.DISABLE_EXAMPLES:
        bld.recurse('examples')
    if not bld.env.DISABLE_BENCHMARKS:
        bld.recurse('benchmark')

    for i in os.listdir('xsd'):
        bld.install_files('${PREFIX}/share/libdcp/xsd', os.path.join('xsd', i))

    for i in ['language', 'region', 'script', 'variant', 'extlang', 'dcnc']:
        bld.install_files('${PREFIX}/share/libdcp/tags', os.path.join('tags', i))

    bld.add_post_fun(post)

def dist(ctx):
    ctx.excl = 'TODO core *~ .git build .waf* .lock* doc/*~ src/*~ test/ref/*~ __pycache__ GPATH GRTAGS GSYMS GTAGS'

def create_version_cc(bld, version):
    if os.path.exists('.git'):
        cmd = "LANG= git log --abbrev HEAD^..HEAD ."
        output = subprocess.Popen(cmd, shell=True, stderr=subprocess.STDOUT, stdout=subprocess.PIPE).communicate()[0].splitlines()
        o = output[0].decode('utf-8')
        commit = o.replace ("commit ", "")[0:10]
    else:
        commit = "release"

    try:
        text =  '#include "version.h"\n'
        text += 'char const * dcp::git_commit = \"%s\";\n' % commit
        text += 'char const * dcp::version = \"%s\";\n' % version
        if bld.env.ENABLE_DEBUG:
            debug_string = 'true'
        else:
            debug_string = 'false'
        text += 'bool const dcp::built_with_debug = %s;\n' % debug_string
        print('Writing version information to src/version.cc')
        o = open('src/version.cc', 'w')
        o.write(text)
        o.close()
    except IOError:
        print('Could not open src/version.cc for writing\n')
        sys.exit(-1)

def post(ctx):
    if ctx.cmd == 'install':
        ctx.exec_command('/sbin/ldconfig')

def tags(bld):
    os.system('etags src/*.cc src/*.h')

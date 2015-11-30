import subprocess
import os
import sys
import distutils.spawn
from waflib import Logs

APPNAME = 'libdcp'
VERSION = '1.2.9devel'
API_VERSION = '-1.0'

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--target-windows', action='store_true', default=False, help='set up to do a cross-compile to Windows')
    opt.add_option('--enable-debug', action='store_true', default=False, help='build with debugging information and without optimisation')
    opt.add_option('--static', action='store_true', default=False, help='build libdcp and in-tree dependencies statically, and link statically to openjpeg and cxml')
    opt.add_option('--disable-tests', action='store_true', default=False, help='disable building of tests')
    opt.add_option('--disable-gcov', action='store_true', default=False, help='don''t use gcov in tests')
    opt.add_option('--disable-examples', action='store_true', default=False, help='disable building of examples')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Wextra', '-D_FILE_OFFSET_BITS=64', '-D__STDC_FORMAT_MACROS'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    conf.env.TARGET_WINDOWS = conf.options.target_windows
    conf.env.TARGET_OSX = sys.platform == 'darwin'
    conf.env.ENABLE_DEBUG = conf.options.enable_debug
    conf.env.DISABLE_TESTS = conf.options.disable_tests
    conf.env.DISABLE_EXAMPLES = conf.options.disable_examples
    conf.env.STATIC = conf.options.static
    conf.env.API_VERSION = API_VERSION

    if conf.options.target_windows:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_WINDOWS')
    else:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_POSIX')

    if not conf.env.TARGET_OSX:
        conf.env.append_value('CXXFLAGS', ['-Wno-unused-result', '-Wno-unused-parameter'])

    conf.check_cfg(package='openssl', args='--cflags --libs', uselib_store='OPENSSL', mandatory=True)
    conf.check_cfg(package='libxml++-2.6', args='--cflags --libs', uselib_store='LIBXML++', mandatory=True)
    conf.check_cfg(package='xmlsec1', args='--cflags --libs', uselib_store='XMLSEC1', mandatory=True)
    # Remove erroneous escaping of quotes from xmlsec1 defines
    conf.env.DEFINES_XMLSEC1 = [f.replace('\\', '') for f in conf.env.DEFINES_XMLSEC1]

    # ImageMagick / GraphicsMagick
    if distutils.spawn.find_executable('Magick++-config'):
        conf.check_cfg(package='', path='Magick++-config', args='--cppflags --cxxflags --libs', uselib_store='MAGICK', mandatory=True)
    else:
        image = conf.check_cfg(package='ImageMagick++', args='--cflags --libs', uselib_store='MAGICK', mandatory=False)
        graphics = conf.check_cfg(package='GraphicsMagick++', args='--cflags --libs', uselib_store='MAGICK', mandatory=False)
        if image is None and graphics is None:
            Logs.pprint('RED', 'Neither ImageMagick++ nor GraphicsMagick++ found: one or the other is required')

    conf.check_cfg(package='sndfile', args='--cflags --libs', uselib_store='SNDFILE', mandatory=False)

    if conf.options.static:
        conf.check_cc(fragment="""
                     #include <stdio.h>\n
                     #include <openjpeg.h>\n
                     int main () {\n
                     void* p = (void *) opj_image_create;\n
                     return 0;\n
                     }
                     """,
                       msg='Checking for library openjpeg', stlib='openjpeg', uselib_store='OPENJPEG', mandatory=True)

        conf.env.HAVE_CXML = 1
        conf.env.STLIB_CXML = ['cxml']
    else:
        conf.check_cfg(package='libopenjpeg', args='--cflags --libs', uselib_store='OPENJPEG', mandatory=True)
        conf.check_cfg(package='libcxml', atleast_version='0.14.0', args='--cflags --libs', uselib_store='CXML', mandatory=True)

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

    conf.check_cxx(fragment="""
                            #include <boost/version.hpp>\n
                            #if BOOST_VERSION < 104500\n
                            #error boost too old\n
                            #endif\n
                            int main(void) { return 0; }\n
                            """,
                   mandatory=True,
                   msg='Checking for boost library >= 1.45',
                   okmsg='yes',
                   errmsg='too old\nPlease install boost version 1.45 or higher.')

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
    conf.recurse('asdcplib')

def build(bld):
    create_version_cc(bld, VERSION)

    if bld.env.TARGET_WINDOWS:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    bld(source='libdcp%s.pc.in' % bld.env.API_VERSION,
        version=VERSION,
        includedir='%s/include/libdcp%s' % (bld.env.PREFIX, bld.env.API_VERSION),
        libs="-L${libdir} -ldcp%s -lasdcp-libdcp%s -lkumu-libdcp%s -lcxml -lboost_system%s" % (API_VERSION, bld.env.API_VERSION, bld.env.API_VERSION, boost_lib_suffix),
        install_path='${LIBDIR}/pkgconfig')

    bld.recurse('src')
    bld.recurse('tools')
    if not bld.env.DISABLE_TESTS:
        bld.recurse('test')
    bld.recurse('asdcplib')
    if not bld.env.DISABLE_EXAMPLES:
        bld.recurse('examples')

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

import subprocess
import os

APPNAME = 'libdcp'
VERSION = '0.49pre'

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--target-windows', action='store_true', default = False, help = 'set up to do a cross-compile to Windows')
    opt.add_option('--enable-debug', action='store_true', default = False, help = 'build with debugging information and without optimisation')
    opt.add_option('--static-openjpeg', action='store_true', default = False, help = 'link statically to openjpeg')
    opt.add_option('--static-libdcp', action='store_true', default = False, help = 'build libdcp and in-tree dependencies statically')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Wextra', '-Wno-unused-result', '-O2', '-D_FILE_OFFSET_BITS=64'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    conf.env.TARGET_WINDOWS = conf.options.target_windows
    conf.env.STATIC_OPENJPEG = conf.options.static_openjpeg
    conf.env.STATIC_LIBDCP = conf.options.static_libdcp
    conf.env.ENABLE_DEBUG = conf.options.enable_debug

    if conf.options.target_windows:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_WINDOWS')
    else:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_POSIX')

    conf.check_cfg(package = 'openssl', args = '--cflags --libs', uselib_store = 'OPENSSL', mandatory = True)
    conf.check_cfg(package = 'libxml++-2.6', args = '--cflags --libs', uselib_store = 'LIBXML++', mandatory = True)
    if conf.options.static_openjpeg:

        conf.check_cc(fragment = """
                       #include <stdio.h>\n
                       #include <openjpeg.h>\n
                       int main () {\n
                       void* p = (void *) opj_image_create;\n
                       return 0;\n
                       }
                       """,
                       msg = 'Checking for library openjpeg', stlib = 'openjpeg', uselib_store = 'OPENJPEG', mandatory = True)
    else:
        conf.check_cfg(package = 'libopenjpeg', args = '--cflags --libs', uselib_store = 'OPENJPEG', mandatory = True)

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

    conf.check_cxx(fragment = """
                              #include <boost/version.hpp>\n
                              #if BOOST_VERSION < 104500\n
                              #error boost too old\n
                              #endif\n
                              int main(void) { return 0; }\n
                              """,
                   mandatory = True,
                   msg = 'Checking for boost library >= 1.45',
                   okmsg = 'yes',
                   errmsg = 'too old\nPlease install boost version 1.45 or higher.')

    conf.check_cxx(fragment = """
    			      #include <boost/filesystem.hpp>\n
    			      int main() { boost::filesystem::copy_file ("a", "b"); }\n
			      """,
                   msg = 'Checking for boost filesystem library',
                   libpath = '/usr/local/lib',
                   lib = ['boost_filesystem%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store = 'BOOST_FILESYSTEM')

    conf.check_cxx(fragment = """
    			      #include <boost/signals2.hpp>\n
    			      int main() { boost::signals2::signal<void (int)> x; }\n
			      """,
                   msg = 'Checking for boost signals2 library',
                   uselib_store = 'BOOST_SIGNALS2')

    conf.recurse('test')
    conf.recurse('asdcplib')

def build(bld):
    create_version_cc(bld, VERSION)

    if bld.env.TARGET_WINDOWS:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    bld(source = 'libdcp.pc.in',
        version = VERSION,
        includedir = '%s/include' % bld.env.PREFIX,
        libs = "-L${libdir} -ldcp -lasdcp-libdcp -lkumu-libdcp -lboost_system%s" % boost_lib_suffix,
        install_path = '${LIBDIR}/pkgconfig')

    bld.recurse('src')
    bld.recurse('tools')
    bld.recurse('test')
    bld.recurse('asdcplib')
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
        text += 'char const * libdcp::git_commit = \"%s\";\n' % commit
        text += 'char const * libdcp::version = \"%s\";\n' % version
        if bld.env.ENABLE_DEBUG:
            debug_string = 'true'
        else:
            debug_string = 'false'
        text += 'bool const built_with_debug = %s;\n' % debug_string
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

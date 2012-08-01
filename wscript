import subprocess
import os

APPNAME = 'libdcp'
VERSION = '0.08pre'

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--target-windows', action='store_true', default = False, help = 'set up to do a cross-compile to Windows')
    opt.add_option('--enable-debug', action='store_true', default = False, help = 'build with debugging information and without optimisation')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Wextra', '-Wno-unused-result', '-O2', '-D_FILE_OFFSET_BITS=64'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    if conf.options.target_windows:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_WINDOWS')
    else:
        conf.env.append_value('CXXFLAGS', '-DLIBDCP_POSIX')

    conf.check_cfg(package = 'openssl', args = '--cflags --libs', uselib_store = 'OPENSSL', mandatory = True)
    conf.check_cfg(package = 'sigc++-2.0', args = '--cflags --libs', uselib_store = 'SIGC++', mandatory = True)
    conf.check_cfg(package = 'libxml++-2.6', args = '--cflags --libs', uselib_store = 'LIBXML++', mandatory = True)

    conf.check_cc(fragment  = """
    			      #include <stdio.h>\n
			      #include <openjpeg.h>\n
			      int main () {\n
			      void* p = (void *) opj_image_create;\n
			      return 0;\n
			      }
			      """, msg = 'Checking for library openjpeg', lib = 'openjpeg', uselib_store = 'OPENJPEG')

    if conf.options.target_windows:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    if conf.options.enable_debug:
        conf.env.append_value('CXXFLAGS', '-g')
    else:
        conf.env.append_value('CXXFLAGS', '-O3')

    conf.check_cxx(fragment = """
    			      #include <boost/filesystem.hpp>\n
    			      int main() { boost::filesystem::copy_file ("a", "b"); }\n
			      """,
                   msg = 'Checking for boost filesystem library',
                   libpath = '/usr/local/lib',
                   lib = ['boost_filesystem%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store = 'BOOST_FILESYSTEM')

    conf.recurse('test')
    conf.recurse('asdcplib')

def build(bld):
    create_version_cc(VERSION)

    bld(source = 'libdcp.pc.in',
        version = VERSION,
        includedir = '%s/include' % bld.env.PREFIX,
        libs = "-L${libdir} -ldcp -lkumu-libdcp -lasdcp-libdcp",
        install_path = '${LIBDIR}/pkgconfig')

    bld.recurse('src')
    bld.recurse('tools')
    bld.recurse('test')
    bld.recurse('asdcplib')

def dist(ctx):
    ctx.excl = 'TODO core *~ .git build .waf* .lock* doc/*~ src/*~ test/ref/*~'

def create_version_cc(version):
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
        print('Writing version information to src/version.cc')
        o = open('src/version.cc', 'w')
        o.write(text)
        o.close()
    except IOError:
        print('Could not open src/version.cc for writing\n')
        sys.exit(-1)

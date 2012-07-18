APPNAME = 'libdcp'
VERSION = '0.01'

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Werror', '-Wextra', '-O2', '-D_FILE_OFFSET_BITS=64'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    conf.check_cfg(package = 'openssl', args = '--cflags --libs', uselib_store = 'OPENSSL', mandatory = True)
    conf.check_cfg(package = 'sigc++-2.0', args = '--cflags --libs', uselib_store = 'SIGC++', mandatory = True)
    
    conf.check_cxx(fragment = """
    			      #include <boost/filesystem.hpp>\n
    			      int main() { boost::filesystem::copy_file ("a", "b"); }\n
			      """,
                   msg = 'Checking for boost filesystem library',
                   libpath = '/usr/local/lib',
                   lib = ['boost_filesystem', 'boost_system'],
                   uselib_store = 'BOOST_FILESYSTEM')

    conf.recurse('test')
    conf.recurse('asdcplib')

def build(bld):
    bld(source = 'libdcp.pc.in',
        version = VERSION,
        includedir = '%s/include' % bld.env.PREFIX,
        libs = "-L${libdir} -ldcp -lkumu-libdcp -lasdcp-libdcp",
        install_path = '${LIBDIR}/pkgconfig')

    bld.recurse('src')
    bld.recurse('test')
    bld.recurse('asdcplib')

def dist(ctx):
    ctx.excl = 'TODO core *~ .git build .waf* .lock* doc/*~ src/*~ test/ref/*~'

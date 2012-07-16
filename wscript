APPNAME = 'libdcp'
VERSION = '0.01pre'

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Werror', '-Wextra', '-O2', '-D_FILE_OFFSET_BITS=64'])
    conf.env.append_value('CXXFLAGS', ['-DLIBDCP_VERSION="%s"' % VERSION])

    conf.check_cc(msg = 'Checking for libkumu',
                  function_name = 'Kumu::Version',
                  header_name = 'KM_util.h',
                  lib = 'kumu',
                  uselib_store = 'KUMU',
                  mandatory = True)

    conf.check_cc(msg = 'Checking for asdcplib',
                  function_name = 'ASDCP::Version',
                  header_name = 'AS_DCP.h',
                  lib = 'asdcp',
                  uselib_store = 'ASDCP',
                  mandatory = True)

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

def build(bld):
    bld.recurse('src')
    bld.recurse('test')


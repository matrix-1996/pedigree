#!/usr/bin/env python2.7
'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'''

from __future__ import print_function

import commands
import getpass
import os
import re
import SCons
import sys

from socket import gethostname
from datetime import datetime

from buildutils import misc
from defaults import *

opts = Variables('options.cache')
opts.AddVariables(
    ('CROSS', 'Base for cross-compilers, tool names will be appended automatically.', ''),
    ('CC', 'Sets the C compiler to use.'),
    ('CXX', 'Sets the C++ compiler to use.'),
    ('AS', 'Sets the assembler to use.'),
    ('AR', 'Sets the archiver to use.'),
    ('LINK', 'Sets the linker to use.'),
    ('STRIP', 'Path to the `strip\' executable.'),
    ('OBJCOPY', 'Path to `objcopy\' executable.'),
    ('CFLAGS', 'Sets the C compiler flags.', ''),
    ('CCFLAGS', 'Sets the C/C++ generic compiler flags.', ''),
    ('CXXFLAGS', 'Sets the C++ compiler flags.', ''),
    ('ASFLAGS', 'Sets the assembler flags.', ''),
    ('LINKFLAGS', 'Sets the linker flags', ''),
    ('BUILDDIR', 'Directory to place build files in.','build'),

    # Controls for the build itself. Will bomb out on bad combinations - will
    # not try and fix them for the user. This is intentional. Note that a
    # fully functional disk image build depends on everything here.
    BoolVariable('build_tests_only', 'Build unit tests ONLY (NOTHING else will be built).', 0),
    BoolVariable('build_kernel_only', 'Build the kernel ONLY (forces all other build_*s to 0).', 0),
    BoolVariable('build_kernel', 'Build the kernel.', 1),
    BoolVariable('build_configdb', 'Build the configuration database (requires `sqlite3`).', 1),
    BoolVariable('build_modules', 'Build drivers, subsystems, and system modules (requires a useful `tar`).', 1),
    BoolVariable('build_lgpl', 'Build LGPL libraries (e.g., libSDL).', 1),
    BoolVariable('build_apps', 'Build in-tree applications (e.g., `ttyterm`).', 1),
    BoolVariable('build_libs', 'Build in-tree libraries (e.g., `libui`).', 1),
    BoolVariable('build_images', 'Build disk images (ISOs + HDD images) - requires all build_* variables.', 1),
    BoolVariable('build_translations', 'Create/update POT/PO files, and build MO files from PO files.', 1),
    BoolVariable('with_graphical_apps', 'Enable graphical apps (e.g. window manager, terminal).', 1),

    # Controls for logging facilities throughout the kernel.
    BoolVariable('posix_verbose', 'Enable verbose logging for all POSIX syscalls.', 0),
    BoolVariable('posix_file_verbose', 'Enable verbose logging for POSIX file syscalls.', 0),
    BoolVariable('posix_sys_verbose', 'Enable verbose logging for POSIX system syscalls.', 0),
    BoolVariable('posix_thr_verbose', 'Enable verbose logging for POSIX pthread syscalls.', 0),
    BoolVariable('posix_net_verbose', 'Enable verbose logging for POSIX network syscalls.', 0),
    BoolVariable('posix_subsys_verbose', 'Enable verbose logging for the PosixSubsystem class.', 0),
    BoolVariable('posix_sig_verbose', 'Enable verbose logging for POSIX signal syscalls.', 0),
    BoolVariable('posix_sig_ultra_verbose', 'Enable even more verbose logging for POSIX signal syscalls.', 0),
    BoolVariable('posix_syscall_verbose', 'Enable logging of every incoming POSIX syscall (PID + syscall number).', 0),

    BoolVariable('posix_musl', 'Build and use musl instead of newlib.', 1),

    # General-purpose configuration knobs.
    BoolVariable('cripple_hdd','Disable writing to hard disks at runtime.',1),
    BoolVariable('debugger','Whether or not to enable the kernel debugger.',1),
    BoolVariable('asserts','Whether or not to enable runtime assertions.',1),
    BoolVariable('debug_logging','Whether to enable debug-level logging, which can dump massive amounts of data to the kernel log. Probable performance hit too, use at your own risk.',1),
    BoolVariable('superdebug', 'Super debug is like the debug_logging option, except even MORE verbose. Expect hundreds of thousands of lines of output to the kernel log.', 0),
    
    BoolVariable('usb_verbose_debug','When set, USB low-level drivers will dump massive amounts of debug information to the log. When not set, only layers such as mass storage will.',0),

    BoolVariable('verbose','Display verbose build output.',0),
    BoolVariable('nocolour','Don\'t use colours in build output.',0),
    BoolVariable('verbose_link','Display verbose linker output.',0),
    BoolVariable('warnings', 'If nonzero, compilation without -Werror', 0),
    BoolVariable('installer', 'Build the installer', 0),
    BoolVariable('genflags', 'Whether or not to generate flags and things dynamically.', 1),
    BoolVariable('ccache', 'Prepend ccache to cross-compiler paths (for use with CROSS)', 0),
    BoolVariable('distcc', 'Prepend distcc to cross-compiler paths (for use with CROSS)', 0),
    BoolVariable('pyflakes', 'Set to one to run pyflakes over Python scripts in the tree', 0),
    BoolVariable('sconspyflakes', 'Set to one to run pyflakes over SConstruct/SConscripts in the tree', 0),
    BoolVariable('travis', 'Set to one/true to indicate that this is a build on Travis-CI.', 0),
    BoolVariable('bochs', 'Set to one/true to change some defaults to improve performance under Bochs.', 0),
    ('iwyu', 'If set, use the given as a the C++ compiler for include-what-you-use. Use -i with scons if you use IWYU.', ''),
    
    BoolVariable('cache', 'Cache SCons options across builds (highly recommended).', 1),
    BoolVariable('genversion', 'Whether or not to regenerate Version.cc if it already exists.', 1),
    
    ('distdir', 'Directory to install a Pedigree directory structure to, instead of a disk image. Empty will use disk images.', ''),
    BoolVariable('forcemtools', 'Force use of mtools (and the FAT filesystem) even if losetup is available.', 0),
    BoolVariable('createvdi', 'Convert the created hard disk image to a VDI file for VirtualBox after it is created.', 0),
    BoolVariable('createvmdk', 'Convert the created hard disk image to a VMDK file for VMware after it is created.', 0),
    BoolVariable('createqcow', 'Convert the created hard disk image to a QCOW2 file after it is created.', 0),
    BoolVariable('iso', 'Building a bootable ISO.', 1),
    BoolVariable('livecd', 'Whether or not the bootable ISO should be a Live CD (one with a disk image on it).', 1),
    
    BoolVariable('pup', 'If 1, you are managing your images/local directory with the Pedigree UPdater (pup) and want that instead of the images/<arch> directory.', 1),
    
    BoolVariable('serial_is_file', 'If 1, the serial port is connected to a file (ie, an emulated serial port). If zero, the serial port is connected to a VT100-compatible terminal.', 1),
    BoolVariable('ipv4_forwarding', 'If 1, enable IPv4 forwarding.', '0'),
    
    BoolVariable('enable_ctrlc', 'If 1, the ability to use CTRL-C to kill running tasks is enabled.', 1),
    BoolVariable('multiple_consoles', 'If 1, the TUI is built with the ability to create and move between multiple virtual consoles.', 1),
    BoolVariable('memory_log', 'If 1, memory logging on the second serial line is enabled.', 1),
    BoolVariable('memory_log_inline', 'If 1, memory logging will be output alongside conventional serial output.', 0),
    BoolVariable('memory_tracing', 'If 1, trace memory allocations and frees (for statistics and for leak detection) on the second serial line. EXCEPTIONALLY SLOW.', 0),
    BoolVariable('track_locks', 'If 1, track spinlocks and ensure they are released in the order they are acquired. Also does some basic deadlock detection.', 1),
    
    BoolVariable('multiprocessor', 'If 1, multiprocessor support is compiled in to the kernel.', 1),
    BoolVariable('apic', 'If 1, APIC support will be built in (not to be confused with ACPI).', 0),
    BoolVariable('acpi', 'If 1, ACPI support will be built in (not to be confused with APIC).', 0),
    BoolVariable('smp', 'If 1, SMP support will be built in.', 1),
    
    BoolVariable('nogfx', 'If 1, the standard 80x25 text mode will be used. Will not load userspace if set to 1.', 0),

    # PC architecture options.
    BoolVariable('mach_pc', 'Target a typical PC architecture.', 0),

    # ARM options
    BoolVariable('arm_integrator', 'Target the Integrator/CP development board', 0),
    BoolVariable('arm_versatile', 'Target the Versatile/PB development board', 0),
    BoolVariable('arm_beagle', 'Target the BeagleBoard', 0),

    BoolVariable('arm_9', 'Target the ARM9 processor family (currently only ARM926E)', 0),
    BoolVariable('armv7', 'Target the ARMv7 architecture family', 0),

    BoolVariable('arm_cortex_a8', 'Tune and optimise for the ARM Cortex A8 (use in conjunction with the `armv7\' option.', 0),

    BoolVariable('arm_bigendian', 'Is this ARM target big-endian?', 0),

    BoolVariable('hosted', 'Is this build to run on another host OS?', 0),
    BoolVariable('clang', 'If hosted, should we use clang if it is present (highly recommended)?', 1),
    BoolVariable('clang_cross', 'If not hosted, should we use clang and cross-compile (kernel and modules only)?', 0),
    BoolVariable('sanitizers', 'If hosted, enable sanitizers (eg AddressSanitizer) (highly recommended)?', 0),
    BoolVariable('valgrind', 'If hosted, build for Valgrind?', 0),
    BoolVariable('force_asan', 'Use ASAN even if Valgrind is present.', 0),
    BoolVariable('clang_profile', 'If hosted, use clang instrumentation to profile.', 0),
    BoolVariable('instrumentation', 'Build with function instrumentation (SLOW).', 0),

    # Hosted build flags.
    BoolVariable('hosted_system_malloc', 'Use the system malloc instead of the Pedigree allocator.', 0),

    # analyses and clang
    BoolVariable('clang_analyse', 'If using clang, pass --analyze (only for kernel+modules).', 0),
    BoolVariable('clang_max_pedantry', 'Use -Weverything with clang, with some specific warnings blacklisted (e.g. -Wdocumentation).', 0),

    BoolVariable('kernel_on_disk', 'Put the kernel & needed bits onto hard disk images?', 1),
    BoolVariable('modules_on_disk', 'Put kernel module files onto hard disk images?', 1),

    BoolVariable('pcap', 'Build the PCAP module, which writes all network traffic to a serial port?', 0),

    BoolVariable('optimise_for_size', 'Optimise for size, not speed (e.g. -Os instead of -O3)', 0),
    
    ('uimage_target', 'Where to copy the generated uImage.bin file to.', '~'),
)

# Autogenerated options that should never be edited by hand.
autogen_opts = Variables('.autogen.cache')
autogen_opts.AddVariables(
    ('COMPILER_TARGET', 'Compiler target (eg, i686-pedigree).', None),
    ('COMPILER_VERSION', 'Compiler version (eg, 4.5.1).', None),
    ('CPPDEFINES', 'Final set of preprocessor definitions.', None),
    ('ARCH_TARGET', 'Automatically generated architecture name.', None),
    ('MACH_TARGET', 'Automatically generated machine name.', None),
    ('HOST_PLATFORM', 'Platform for the compile host.', None),
    ('ARCH_DIR', 'Automatically determined directory for architecture files.', None),
    ('SUBARCH_DIR', 'Automatically determined directory for subarchitecture files.', None),
    ('MACH_DIR', 'Automatically determined directory for machine files.', None),
    ('BOOT_DIR', 'Directory in which to find boot-protocol-related files.', None),
    BoolVariable('ON_PEDIGREE', 'Whether we are on Pedigree or not.', False),
    BoolVariable('reset_flags', 'Whether to reset *FLAGS variables or not. '
        'Avoid using where possible. Toggles to False after use.', True),
)

# Load the autogenerated environment first up.
autogen_env = Environment(options=autogen_opts, tools=[])

# Make sure we're running on a sane SCons version (this actually builds
# an environment, as it happens).
EnsureSConsVersion(0, 98, 0)

# If we need to reset flags, we do that now, before loading the main
# environment.
# This is because if we need to reset flags to actually complete a build, we
# can't depend on the environment even initialising properly. For example, if
# the type for a flag changes, any previously-saved value for the flag will
# be invalid.
if autogen_env['reset_flags']:
    autogen_env['reset_flags'] = False

    if os.path.isfile('options.cache'):
        processed_lines = []
        with open('options.cache') as f:
            for line in f:
                fields = [x.strip() for x in line.split('=', 1)]
                if not fields[0].lower().endswith('flags'):
                    processed_lines.append(line)

        with open('options.cache', 'w') as f:
            f.write('\n'.join(processed_lines))

# Explicitly limit the set of tools to try and find instead of using 'default'.
# This stops SCons from trying to find Fortran tools and the like.
tools_to_find = [
    'gnulink', 'gcc', 'g++', 'gas', 'ar', 'textfile', 'filesystem', 'tar',
    'cc', 'c++', 'link', 'gettext'
]

# Some things require either a value, or the key to be not present at all (in
# particular, ccache). So, we fix that up here.
def fix_system_environment(env):
    sentinel = object()
    for value_required_key in ('CCACHE_DIR', 'CCACHE_TEMPDIR'):
        if not env.get(value_required_key, sentinel):
            del env[value_required_key]

    return env

# Copy the host environment and install our options. If we use env.Platform()
# after this point, the Platform() call will override ENV and we don't want that
# or env['ENV']['PATH'] won't be the user's $PATH from the shell environment.
# That specifically breaks the build on OS X when using tar from macports (which
# is needed at least on OS X 10.5 as the OS X tar does not have --transform).
system_path = os.environ.get('PATH', '')
environment = fix_system_environment({
    'PATH': system_path,
    'HOME': os.environ.get('HOME', '/'),
    # Needed to pass in preloads for Bear to build compilation databases.
    'LD_PRELOAD': os.environ.get('LD_PRELOAD', ''),
    'DYLD_INSERT_LIBRARIES': os.environ.get('DYLD_INSERT_LIBRARIES', ''),
    # Pull in extra ccache configuration.
    'CCACHE_DIR': os.environ.get('CCACHE_DIR', ''),
    'CCACHE_TEMPDIR': os.environ.get('CCACHE_DIR', ''),
})

def build_env():
    return Environment(options=opts, platform='posix', ENV=environment,
                       tools=tools_to_find, TARFLAGS='-cz')

try:
    env = build_env()
except SCons.Errors.EnvironmentError:
    tools_to_find.remove('textfile')
    try:
        env = build_env()
    except SCons.Errors.EnvironmentError:
        tools_to_find.remove('gettext')
        env = build_env()
        env['build_translations'] = False
Help(opts.GenerateHelpText(env))

# Load the host environment now - this is a boring, standard environment that
# doesn't need many flags from the main build.
host_environ = {'ENV': environment}
host_cxx = os.environ.get('CXX')
host_cc = os.environ.get('CC')
if host_cxx:
    host_environ['CXX'] = host_cxx
if host_cc:
    host_environ['CC'] = host_cc
host_env = Environment(platform='posix', **host_environ)

# Build a basic set of variables for the configure checks below.
host_env.MergeFlags({
    'CPPPATH': ['/include', '/usr/include', '/usr/local/include'],
    'LIBPATH': ['/lib', '/usr/lib', '/usr/local/lib'],
    })

conf = Configure(host_env)
profile_rt = conf.CheckLib('profile_rt')
gcov = conf.CheckLib('gcov')
conf.env['HAVE_BENCHMARK'] = conf.CheckLibWithHeader('benchmark',
    'benchmark/benchmark.h', 'cxx')
conf.env['HAVE_OPENSSL_SHA'] = conf.CheckLibWithHeader('crypto',
    'openssl/sha.h', 'cxx')
host_env = conf.Finish()

# TODO(miselin): figure out how best to detect asan presence.
host_env['COVERAGE_CCFLAGS'] = ['-fprofile-arcs', '-ftest-coverage', '-O1']
host_env['COVERAGE_LINKFLAGS'] = ['-fprofile-arcs', '-ftest-coverage']
if env['force_asan'] or (host_env.Detect('valgrind') is None):
    host_env['COVERAGE_CCFLAGS'] += ['-fsanitize=address']
    host_env['COVERAGE_LINKFLAGS'] += ['-fsanitize=address']

    host_env.MergeFlags({
        'CCFLAGS': ['-fsanitize=address'],
        'LINKFLAGS': ['-fsanitize=address'],
    })
host_env['HAS_PROFILE_RT'] = profile_rt
host_env['HAS_GCOV'] = gcov

# Copy useful environment items that are needed in site_scons/buildutils
for copy_key in ('verbose', 'nocolour', 'instrumentation', 'memory_tracing'):
    host_env[copy_key] = env[copy_key]

# Perform timestamp checks first, then MD5 checks, to figure out if things change.
env.Decider('MD5-timestamp')
host_env.Decider('MD5-timestamp')

# Avoid any form of RCS scan (git etc)
env.SourceCode('.', None)
host_env.SourceCode('.', None)

# Cache file checksums after 60 seconds
SetOption('max_drift', 60)

# Intelligently cache implicit dependencies (rather than checking them on
# every single build).
SetOption('implicit_cache', 1)

# Restrict suffixes we consider C++ to avoid searching for Fortran, .m, etc
env['CPPSUFFIXES'] = ['.c', '.C', '.cc', '.h', '.hpp', '.cpp', '.S']
host_env['CPPSUFFIXES'] = ['.c', '.C', '.cc', '.h', '.hpp', '.cpp', '.S']

# Allow automatic init of missing .po files.
env['POAUTOINIT'] = True

# Bring across autogen variables to the main environment.
for k in autogen_opts.keys():
    env[k] = autogen_env.get(k)

# Determine whether we're cross-compiling or not.
if env.get('HOST_PLATFORM') is None:
    host = os.uname()
    env['ON_PEDIGREE'] = host[0] == 'Pedigree'
    env['HOST_PLATFORM'] = host[4].lower()

# If we're building on Pedigree, we enforce not building images for now.
if env['ON_PEDIGREE']:
    env['build_images'] = False
    env['build_apps'] = False
    env['build_lgpl'] = False
    env['build_libs'] = False

# Make sure there's a sensible configuration.
if env['build_tests_only']:
    env['build_kernel_only'] = False
    env['build_kernel'] = False
    env['build_configdb'] = False
    env['build_modules'] = False
    env['build_lgpl'] = False
    env['build_apps'] = False
    env['build_libs'] = False
    env['build_images'] = False
    env['ARCH_TARGET'] = 'TESTS'

if env['build_kernel_only']:
    env['build_kernel'] = True
    env['build_configdb'] = False
    env['build_modules'] = False
    env['build_lgpl'] = False
    env['build_apps'] = False
    env['build_libs'] = False
    env['build_images'] = False
if env['build_images']:
    if not all(env[x] for x in ('build_kernel', 'build_configdb',
                                'build_modules', 'build_lgpl', 'build_apps',
                                'build_libs', 'build_images')):
        raise SCons.Errors.UserError('build_images requires all build_* options set to 1.')
if env['build_configdb'] and not env['build_modules']:
    raise SCons.Errors.UserError('build_configdb requires build_modules=1.')


def check_for(env, var, name, bins=None, required=False):
    # Already cached?
    if env.get(var):
        return

    print('Checking for %s... ' % (name,), end='')
    if bins is None:
        bins = name
    env[var] = env.Detect(bins)
    if not env[var]:
        print('not found!')
        if required:
            raise SCons.Errors.UserError('%s is required, but was not found.' % (name,))
    else:
        print('found!')


# Look for things we care about for the build.
check_for(env, 'QEMU_IMG', 'qemu-img')
check_for(env, 'LOSETUP', 'losetup')
check_for(env, 'MKE2FS', 'mke2fs')
check_for(env, 'DEBUGFS', 'debugfs')
check_for(env, 'CCACHE', 'ccache')
check_for(env, 'DISTCC', 'distcc')
check_for(env, 'PYFLAKES', 'pyflakes')
check_for(env, 'MTOOLS_MMD', 'mmd')
check_for(env, 'MTOOLS_MCOPY', 'mcopy')
check_for(env, 'MTOOLS_MDEL', 'mdel')
check_for(env, 'MKISOFS', 'mkisofs', ['mkisofs', 'genisoimage', 'xorriso'])
check_for(env, 'SQLITE', 'sqlite3')
check_for(env, 'MKIMAGE', 'mkimage')
check_for(env, 'GIT', 'git')

# If we're on OSX, make sure we're using a sane tar.
if sys.platform == 'darwin':
    # Don't override an overridden TAR variable.
    if env['TAR'] == 'tar':
        env['TAR'] = env.Detect('gnutar')
        if not env['TAR']:
            # Homebrew gnu-tar installs as gtar rather than gnutar.
            env['TAR'] = env.Detect('gtar')
            if not env['TAR']:
                # If everything else fails, fall back to BSD tar.
                # TODO(miselin): 'tar' might actually be GNU tar, if $PATH is
                # set to put GNU tools first... we should verify before telling
                # the user we're falling back to BSD tar.
                print('Falling back to BSD tar, please be aware that this is '
                      'an unsupported configuration.')
                print('Please consider installing GNU tar.')
                env['TAR'] = 'tar'

# Look for things we absolutely must have.
required_tools = ['TAR']
if env['build_configdb']:
    # Need sqlite if we're building a config database.
    required_tools.append('SQLITE')
if not all([env[x] is not None for x in required_tools]):
    raise SCons.Errors.UserError('Could not find all needed tools (need: %r)' % required_tools)

# Confirm whether we're making an ISO or not.
if env['MKISOFS'] is None:
    print('No program to generate ISOs, not generating an ISO.')
    env['iso'] = False

# Can we even build an image?
if env['build_images'] and not any([env[x] is not None for x in ['LOSETUP', 'MKE2FS', 'MTOOLS_MMD']]):
    msg = 'Cannot create a disk image by any means.'
    if env['distdir']:
        print(msg)
    else:
        raise SCons.Errors.UserError(msg)

# Enforce pre-commit hook.
if not os.path.exists('.git/hooks/pre-commit'):
    print( "Enforcing pre-commit script.")
    os.symlink('../../scripts/pre-commit.sh', '.git/hooks/pre-commit')

# Reset object file suffixes.
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = ''

# Reset file build suffixes (defaults are not always correct on each platform).
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = ''

# Don't build shared libraries with versions + symlinks.
# env['SHLIBNOVERSIONSYMLINKS'] = False

# Determine build directories (these can be outside the source tree).
env['BUILDDIR'] = env.Dir(env['BUILDDIR']).abspath  # Normalise path.
env['HOST_BUILDDIR'] = os.path.join(env['BUILDDIR'], 'host')
env['PEDIGREE_BUILD_BASE'] = env['BUILDDIR']
env['PEDIGREE_BUILD_MODULES'] = os.path.join(env['BUILDDIR'], 'modules')
env['PEDIGREE_BUILD_KERNEL'] = os.path.join(env['BUILDDIR'], 'kernel')
env['PEDIGREE_BUILD_DRIVERS'] = os.path.join(env['BUILDDIR'], 'drivers')
env['PEDIGREE_BUILD_SUBSYS'] = os.path.join(env['BUILDDIR'], 'subsystems')
env['PEDIGREE_BUILD_APPS'] = os.path.join(env['BUILDDIR'], 'apps')

# Add host build output path.
host_env['BUILDDIR'] = env['HOST_BUILDDIR']

# Set up compilers and in particular the cross-compile environment.
if (not env['build_tests_only']) and (env['CROSS'] or env['ON_PEDIGREE']):
    if env['ON_PEDIGREE']:
        # TODO: parse 'gcc -v' to get COMPILER_TARGET
        env['COMPILER_TARGET'] = 'FIXME'
        crossPath = '/applications'
        crossTuple = ''
    else:
        cross = os.path.split(env['CROSS'])
        crossPath = cross[0]
        crossTuple = cross[1].strip('-')

        env['COMPILER_TARGET'] = crossTuple
        if 'i686' in crossTuple:
            raise SCons.Errors.UserError('Please run "easy_build_x64.sh" to '
                'create a 64-bit toolchain: 32-bit builds of Pedigree are no '
                'longer supported.')

    tools = {}
    for tool in ['gcc', 'g++', 'as', 'ar', 'ranlib', 'strip', 'objcopy']:
        if crossTuple:
            tool_name = '%s-%s' % (crossTuple, tool)
        else:
            tool_name = tool
        tool_path = os.path.join(os.path.abspath(crossPath), tool_name)
        if os.path.isfile(tool_path):
            tools[tool] = tool_path
        else:
            raise SCons.Errors.UserError('Needed tool "%s" not found for '
                'target "%s".' % (tool, crossTuple))

    # Reset the compiler version as needed.
    # XXX: nasty hack, but older SCons doesn't allow a CC override.
    SCons.Tool.gcc.preserved_compilers = SCons.Tool.gcc.compilers
    SCons.Tool.gcc.compilers = [tools['gcc']]
    env.Tool('gcc')

    env['XCOMPILER_PATH'] = crossPath

    prefix = ''
    if env['distcc'] and env['DISTCC'] is not None:
        prefix = '%s %s' % (env['DISTCC'], prefix)
    if env['ccache'] and env['CCACHE'] is not None:
        prefix = '%s %s' % (env['CCACHE'], prefix)
    prefix = prefix.strip()

    env['CC'] = '%s %s' % (prefix, tools['gcc'])
    env['CXX'] = '%s %s' % (prefix, tools['g++'])
    env['AS'] = tools['as']

    env['AR'] = tools['ar']
    env['RANLIB'] = tools['ranlib']
    env['LINK'] = tools['gcc']
    env['STRIP'] = tools['strip']
    env['OBJCOPY'] = tools['objcopy']

    ccversion = env.get('CCVERSION')
    if not ccversion:
        ccversion = commands.getoutput('%s -dumpversion' % env['CC'])
        env['CCVERSION'] = ccversion

    # Set up target-specific versions of the various tools. These are used
    # where code should only ever be built for Pedigree; for almost every build
    # type they should match the non-target-specific versions. For a hosted
    # build, though, they will not match; e.g. OSX's ld should only be used for
    # binaries, not for e.g. loadable modules.
    for tool in ('CC', 'CXX', 'AS', 'AR', 'RANLIB', 'LINK', 'STRIP',
                 'OBJCOPY'):
        env['TARGET_%s' % tool] = env[tool]

    userspace_env = env
elif not (env['hosted'] or env['build_tests_only']):
    raise SCons.Errors.UserError('No cross-compiler specified and not on '
        'Pedigree, and not building a hosted system. Check flags and try '
        'again.')

if env['hosted']:
    # Do we have a userspace environment?
    try:
        _ = userspace_env
    except NameError:
        raise SCons.Errors.UserError('Hosted builds still require a '
            'cross-compiler to build userspace components.')

    # We fix tools later in SConstruct.
    env['multiprocessor'] = 0  # force-disable MP support on hosted
    env['smp'] = 0

if (not env['build_tests_only']) and (env['ON_PEDIGREE'] or env['COMPILER_TARGET']):
    if env['ON_PEDIGREE'] or env['hosted']:
        host_arch = env['HOST_PLATFORM']
    else:
        host_arch = env['COMPILER_TARGET']

    extra_defines = []
    flags_machine = None
    if re.match('i[3456]86', host_arch) is not None:
        flags_arch = 'x86'

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x86']
        env['ARCH_TARGET'] = 'X86'
        flags_machine = 'pc'
    elif re.match('amd64|x86[_-]64|x64', host_arch) is not None:
        flags_arch = 'x64'

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x64']
        env['ARCH_TARGET'] = 'X64'
        flags_machine = 'pc'
    elif re.match('ppc|powerpc', host_arch) is not None:
        flags_arch = 'ppc'

        extra_defines += ['PPC']
        env['ARCH_TARGET'] = 'PPC'
        flags_machine = 'mac'
    elif re.match('arm', host_arch) is not None:
        flags_arch = 'arm'

        # Handle input options
        mach = ''
        if env['arm_integrator']:
            extra_defines += ['ARM_INTEGRATOR']
            mach = 'integrator'
        elif env['arm_versatile']:
            extra_defines += ['ARM_VERSATILE']
            mach = 'versatile'
        elif env['arm_beagle']:
            extra_defines += ['ARM_BEAGLE']
            mach = 'beagle'

        flags_machine = mach

        ccflags = default_flags['arm']
        cflags = default_cflags['arm']
        cxxflags = default_cxxflags['arm']
        default_linkflags['arm'] = [x.replace('[mach]', mach) for x in
                                    default_linkflags['arm']]

        if env['arm_9']:
            extra_defines += ['ARM926E'] # TODO: too specific.
        elif env['armv7']:
            extra_defines += ['ARMV7']
            if env['arm_cortex_a8']:
                # TODO: actually need to use VFP, not FPA
                arm_flags = ['-mcpu=cortex-a8', '-mtune=cortex-a8', '-mfpu=vfp']
                ccflags += arm_flags

        default_flags['arm'] = ccflags
        default_cflags['arm'] = cflags
        default_cxxflags['arm'] = cxxflags

        if env['arm_bigendian']:
            extra_defines += ['TARGET_IS_BIG_ENDIAN']
        else:
            extra_defines += ['TARGET_IS_LITTLE_ENDIAN']

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['arm']
        env['ARCH_TARGET'] = 'ARM'

    flags = default_flags.get(flags_arch)
    if flags is not None:
        # Fix up optimisation flags.
        if env['optimise_for_size']:
            default_flags[flags_arch] = [
                x.replace('-O3', '-Os') for x in default_flags[flags_arch]]

        mapping = {
            'CCFLAGS': default_flags[flags_arch],
            'CFLAGS': default_cflags[flags_arch],
            'CXXFLAGS': default_cxxflags[flags_arch],
            'ASFLAGS': default_asflags[flags_arch],
            'LINKFLAGS': default_linkflags[flags_arch],
        }

        for k in mapping:
            try:
                # Force a recreation of the flags.
                del env[k]
            except KeyError:
                # Doesn't exist.
                pass

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir[flags_arch]
        defines = default_defines[flags_arch] + extra_defines

        env.MergeFlags(mapping)
        userspace_env.MergeFlags(mapping)
    else:
        defines = generic_defines

    env['EXTRA_CONFIG'] = default_extra_config.get(flags_arch, [])
    env['ARCH_DIR'] = default_arch_dir.get(flags_arch)
    env['SUBARCH_DIR'] = default_subarch_dir.get(flags_arch)
    if flags_machine:
        env['MACH_TARGET'] = flags_machine
        env['MACH_DIR'] = default_machine_dir.get(flags_machine)
    env['BOOT_DIR'] = target_boot_directory.get(flags_arch)

    for key, override_value in environment_overrides.get(flags_arch, {}).items():
        env[key] = override_value
else:
    defines = []

# Handle no valid target sensibly.
if (not all([env['ARCH_TARGET'], env['ARCH_DIR'], env['MACH_DIR']]) and
        not env['ON_PEDIGREE']) and (not env['build_tests_only']):
    raise SCons.Errors.UserError('Unsupported target - have you used '
        'scripts/checkBuildSystem.pl to build a cross-compiler?')

# Machine selection (this almost certainly shouldn't be here).
# TODO(miselin) figure out how to do this better
if env['ARCH_TARGET'] in ['X86', 'X64']:
    env['mach_pc'] = 1

# Override images directory if Pup is desired.
if env['pup']:
    env['PEDIGREE_IMAGES_DIR'] = '#images/local'

    imagesdir = env.Dir(env['PEDIGREE_IMAGES_DIR']).abspath
    if not os.path.isdir(imagesdir):
        os.makedirs(imagesdir)

# NASM is used for X86 and X64 builds
if env['ARCH_TARGET'] in ('X86', 'X64'):
    env['AS'] = None
    if env['ON_PEDIGREE']:
        env['AS'] = env.Detect('nasm')
    else:
        env['AS'] = os.path.join(env['XCOMPILER_PATH'], 'nasm')
if env['AS'] is None:
    raise SCons.Errors.UserError('No assembler was found - make sure nasm/as are installed.')

# Adjust build options for ARM, which is not quite there yet in terms of
# having a fully functional userspace.
if env['ARCH_TARGET'] == 'ARM':
    env['iso'] = False
    env['build_images'] = False
    env['build_lgpl'] = False
    env['build_libs'] = False
    env['build_apps'] = False

# Add optional flags.
warning_flag = ['-Wno-error']
if not env['warnings']:
    warning_flag = ['-Werror']
env.MergeFlags({'CCFLAGS': warning_flag})

# Instrumentation conflicts with memory logging.
if env['memory_log'] and not env['instrumentation']:
    defines.append('MEMORY_LOGGING_ENABLED')

def fixDebugFlags(environment):
    # Handle extra debugging components.
    if not environment['debugger']:
        return

    # Build in debugging information when built with the debugger.
    # Use DWARF, as the stabs format is not very useful (32 bits of reloc)
    debug_flags = {'CCFLAGS': ['-g3', '-ggdb', '-gdwarf-2']}
    environment.MergeFlags(debug_flags)

    if 'nasm' not in environment['AS']:
        debug_flags = {'ASFLAGS': debug_flags['CCFLAGS']}
        environment.MergeFlags(debug_flags)
fixDebugFlags(env)

additionalDefines = ['ipv4_forwarding', 'serial_is_file', 'installer',
                     'debugger', 'cripple_hdd', 'enable_ctrlc',
                     'multiple_consoles', 'multiprocessor', 'smp', 'apic',
                     'acpi', 'debug_logging', 'superdebug', 'nogfx', 'mach_pc',
                     'usb_verbose_debug', 'travis', 'hosted',
                     'memory_log_inline', 'asserts', 'valgrind', 'livecd']
for i in additionalDefines:
    if i not in env:
        continue

    if env[i]:
        defines += [i.upper()]

# TODO(miselin): figure out how to do dependent flags
if(env['multiprocessor'] or env['smp']):
    defines += ['MULTIPROCESSOR', 'APIC', 'ACPI', 'SMP']

# Set the environment flags
env['CPPDEFINES'] = list(set(defines))

# Grab the date (rather than using the `date' program)
env['PEDIGREE_BUILDTIME'] = datetime.today().isoformat()

# Use the OS to find out information about the user and computer name
env['PEDIGREE_USER'] = getpass.getuser()
env['PEDIGREE_MACHINE'] = gethostname() # The name of the computer (not the type or OS)

# Grab the git revision of the repo
if env['GIT']:
    env['PEDIGREE_REVISION'] = commands.getoutput('%s rev-parse --verify HEAD --short' % env['GIT'])
else:
    env['PEDIGREE_REVISION'] = "(unknown)"

# Set the flags
env['PEDIGREE_FLAGS'] = ' '.join(env['CPPDEFINES'])

version_out = ['#include <Version.h>',
               'const char *g_pBuildTime = "$buildtime";',
               'const char *g_pBuildRevision = "$rev";',
               'const char *g_pBuildFlags = "$flags";',
               'const char *g_pBuildUser = "$user";',
               'const char *g_pBuildMachine = "$machine";',
               'const char *g_pBuildTarget = "$target";']

sub_dict = {"$buildtime"    : env['PEDIGREE_BUILDTIME'],
            "$rev"          : env['PEDIGREE_REVISION'],
            "$flags"        : env['PEDIGREE_FLAGS'],
            "$user"         : env['PEDIGREE_USER'],
            "$machine"      : env['PEDIGREE_MACHINE'],
            "$target"       : env['ARCH_TARGET']
           }

# Create the file
def create_version_cc(target, source, env):
    global version_out

    # We need to have a Version.cc, but we can disable the (costly) rebuild of
    # it every single time a compile is done - handy for developers.
    if (not env['genversion']) and os.path.exists(target[0].abspath):
        return

    # Make the non-SCons target a bit special.
    # People using Cygwin have enough to deal with without boring
    # status messages from build systems that don't support fancy
    # builders to do stuff quickly and easily.
    info = "Version.cc [rev: %s, with: %s@%s]" % (env['PEDIGREE_REVISION'], env['PEDIGREE_USER'], env['PEDIGREE_MACHINE'])
    if env['verbose']:
        print("      Creating %s" % (info,))
    else:
        print('      Creating \033[32m%s\033[0m' % (info,))

    def replacer(s):
        for keyname, value in sub_dict.iteritems():
            s = s.replace(keyname, value)
        return s

    version_out = map(replacer, version_out)

    f = open(target[0].path, 'w')
    f.write('\n'.join(version_out))
    f.write('\n')
    f.close()

env.Command(os.path.join(env['PEDIGREE_BUILD_BASE'], 'Version.cc'), None, Action(create_version_cc, None))

# Preserve compilation flags for the target.
for key in ('CFLAGS', 'CCFLAGS', 'CXXFLAGS', 'LINKFLAGS'):
    env['TARGET_%s' % key] = env[key]

if env['hosted']:
    userspace_env = env.Clone()

    # Remove multiprocessor-related flags (not sensible for hosted targets).
    '''
    env['CPPDEFINES'] = misc.removeFlags(
        env['CPPDEFINES'],
        ['MULTIPROCESSOR', 'APIC', 'ACPI', 'SMP']
    )
    '''

    # Fix tools to use host.
    SCons.Tool.gcc.compilers = SCons.Tool.gcc.preserved_compilers
    for hosted_tool in reversed(tools_to_find):
        env.Tool(hosted_tool)

    env['COMPILER_TARGET'] = 'HOSTED'

    removal_defines = ('X86', 'X64', 'X86_COMMON')
    env['CPPDEFINES'] = [x for x in env['CPPDEFINES'] if x not in removal_defines]
    env['CPPDEFINES'] += ['__pedigree__']

    removal_flags = ('-mcmodel=kernel',)
    for key in ('CCFLAGS', 'TARGET_CCFLAGS'):
        env[key] = [x for x in env[key] if x not in removal_flags]

    # Copy across userspace defines as HOSTED_*
    env['CPPDEFINES'] += ['HOSTED_%s' % x for x in userspace_env['CPPDEFINES']]

    # setjmp/longjmp context switching - we can't return from the function
    # calling setjmp without invoking undefined behaviour.
    # Also note: we emulate multiboot for the hosted "boot" protocol.
    env['CPPDEFINES'] += ['SYSTEM_REQUIRES_ATOMIC_CONTEXT_SWITCH', 'MULTIBOOT']

    # Reset flags.
    env['CCFLAGS'] = (generic_flags + warning_flags + warning_flags_off +
                      ['-U_FORTIFY_SOURCE', '-U__linux__'])
    env['CFLAGS'] = generic_cflags + warning_flags_c
    env['CXXFLAGS'] = generic_cxxflags + warning_flags_cxx
    env['LINKFLAGS'] = []
    env['LIBPATH'] = []

    # Don't omit frame pointers for debugging.
    env.MergeFlags({
        'CCFLAGS': ['-fno-omit-frame-pointer', '-Wno-deprecated-declarations'],
    })

    if env['force_asan']:
        env.MergeFlags({
            'CCFLAGS': ['-fsanitize=address'],
            'LINKFLAGS': ['-fsanitize=address'],
        })

    if env['clang_cross'] and env['force_asan']:
        env.MergeFlags({
            'TARGET_CCFLAGS': ['-fsanitize=address'],
            'TARGET_LINKFLAGS': ['-fsanitize=address'],
        })

    if not env['warnings']:
        env.MergeFlags({'CCFLAGS': '-Werror'})

    # Use the large memory model to avoid making any assumptions about the
    # address space (e.g. PC32 relocations).
    env.MergeFlags({
        'CCFLAGS': ['-mcmodel=large', '-g3', '-ggdb', '-pg'],
        'TARGET_CCFLAGS': ['-mcmodel=large', '-g3', '-ggdb', '-pg'],
        'LINKFLAGS': ['-mcmodel=large', '-pg'],
    })

    # Not a PC.
    env['mach_pc'] = False

    # Don't build an ISO, but disk images are okay. Don't put kernel on the
    # disk, as we only want to rebuild it if the files change.
    env['kernel_on_disk'] = False
    env['iso'] = False

    # Fix tar flags to not build compressed tarballs.
    env['TAR_NOCOMPRESS'] = True

    # Fix assembler config.
    if env['ARCH_TARGET'] in ('X86', 'X64'):
        env['AS'] = env.Detect('nasm')
    env['ASFLAGS'] = userspace_env['ASFLAGS']

    # Now ditch any ARCH_TARGET-related hooks - we don't need it anymore.
    env['ARCH_TARGET'] = 'HOSTED'
    env['MACH_TARGET'] = 'hosted'

    env['EXTRA_CONFIG'] = default_extra_config.get('hosted', [])
    env['ARCH_DIR'] = default_arch_dir.get('hosted')
    env['SUBARCH_DIR'] = default_subarch_dir.get('hosted')
    env['MACH_DIR'] = default_machine_dir.get('hosted')
    env['BOOT_DIR'] = target_boot_directory.get('hosted')

    # Save the useful GCC we already have.
    gcc = env['CC']

    # Save path to libstdc++
    libstdcxx_path = commands.getoutput('%s -print-file-name=libstdc++.so' % env['CC'])
    env['LIBPATH'].append(os.path.dirname(libstdcxx_path))

    # Do we have clang?
    if env['clang'] and env.Detect('clang') is not None:
        clang = env.Detect('clang')
        clangxx = env.Detect('clang++')
        if clang and clangxx:
            env['CC'] = clang
            env['CXX'] = clangxx
            env['LINK'] = clangxx

            # Wipe out some warnings that clang can't handle.
            for flag in ('-Wuseless-cast', '-Wsuggest-attribute=noreturn',
                         '-Wtrampolines', '-Wlogical-op',
                         '-Wno-packed-bitfield-compat'):
                for flags in ('CCFLAGS', 'CFLAGS', 'CXXFLAGS'):
                    if flag in env[flags]:
                        env[flags].remove(flag)

            # This manages to differ between at least the clang on Travis and
            # a more recent (built from SVN) clang. So, don't error on it.
            env.MergeFlags({'CCFLAGS': [
                '-Wno-error=implicit-exception-spec-mismatch']})
    else:
        env['clang'] = False
        print('Note: not using clang for hosted build.')

    if env['clang'] and env['sanitizers']:
        sanitizers = (
            'integer',
            'undefined',
            'address',
        )
        sanitizers = '-fsanitize=%s' % ','.join(sanitizers)
        asan_supp = env.File('#ASan.supp').path
        sanitizers = [sanitizers, '-fsanitize-blacklist=%s' % asan_supp]
        env.MergeFlags({'CCFLAGS': sanitizers, 'LINKFLAGS': sanitizers})

    if env['clang'] and env['clang_profile']:
        env['CPPDEFINES'] += ['CLANG_PROFILE']
        env.MergeFlags({'CCFLAGS': ['-fprofile-instr-generate'],
                        'LINKFLAGS': ['-fprofile-instr-generate']})

    fixDebugFlags(env)
else:
    env['clang'] = 0

if env['clang_cross']:  # and not env['hosted']:
    if not env['hosted']:
        userspace_env = env.Clone()

    cross_dir = os.path.dirname(env['CROSS'])
    if cross_dir:
        env.PrependENVPath('PATH', cross_dir)

    orig_link = os.path.basename(env['CC'])

    # Override the main kernel environment, but not the userspace one.
    if not env['hosted']:
        env['CC'] = 'clang'
        env['CXX'] = 'clang++'
        env['LINK'] = 'clang'

    env['TARGET_CC'] = 'clang'
    env['TARGET_CXX'] = 'clang++'
    env['TARGET_LINK'] = 'clang'

    # TODO(miselin): correct triple (e.g. ARM)
    triple = ['-target']
    if env['ARCH_TARGET'] == 'X64':
        triple.append('x86_64-none-elf')
    elif env['ARCH_TARGET'] == 'ARM':
        # Assume armv7
        triple.append('arm-none-eabi')
    else:
        triple = []
    cross_gcc = ['-ccc-gcc-name', orig_link]

    # Generic flags we care about for compilation and linking.
    if env['force_asan']:
        # Some warnings are not compatible with asan.
        generic_flags = []
        generic_ccflags = []
    else:
        generic_flags = ['-Qunused-arguments']
        generic_ccflags = ['-Wno-unused-parameter']

    if env['clang_max_pedantry'] and not env['force_asan']:
        generic_ccflags += ['-Weverything', '-Wno-documentation',
                            '-Wno-documentation-unknown-command',
                            '-Wno-reserved-id-macro', '-Wno-c++98-compat',
                            '-Wno-c++98-compat-pedantic', '-Wno-packed',
                            '-Wno-padded', '-Wno-error=newline-eof',
                            '-Wno-weak-vtables', '-Wno-exit-time-destructors',
                            '-Wno-global-constructors', '-Wno-unused-macros',
                            '-Wno-format', '-Wno-variadic-macros',
                            '-Wno-gnu-anonymous-struct', '-Wno-gnu-include-next',
                            '-Wno-unused-private-field', '-Wno-switch-enum',
                            '-Wno-unused-variable', '-Wno-unused-function',
                            '-Wno-unreachable-code', '-Wno-nested-anon-types',
                            ]

    env['CLANG_BASE_LINKFLAGS'] = triple + cross_gcc + generic_flags

    # Punch out some warning flags that clang doesn't know.
    misc.removeFromAllFlags(env, [
        '-Wuseless-cast', '-Wno-packed-bitfield-compat', '-Wlogical-op',
        '-Wtrampolines', '-Wsuggest-attribute=noreturn', '-mapcs-frame'])

    # Setting unique=0 appends and does not reorder the given arguments, which
    # is crucial as these are "-arg value" style parameters.
    if not env['hosted']:
        env.MergeFlags({
            'CCFLAGS': triple + generic_flags + generic_ccflags,
            'LINKFLAGS': env['CLANG_BASE_LINKFLAGS'],
        }, unique=0)

    env.MergeFlags({
        'TARGET_CCFLAGS': triple + generic_flags + generic_ccflags,
        'TARGET_LINKFLAGS': env['CLANG_BASE_LINKFLAGS'],
    }, unique=0)

    # Do we need to do analysis?
    if env['clang_analyse']:
        env.MergeFlags({
            'CCFLAGS': ['--analyze'],
            'TARGET_CCFLAGS': ['--analyze'],
            'LINKFLAGS': ['--analyze'],
        })

        env['CLANG_BASE_LINKFLAGS'] += ['--analyze']

        # None of the following use clang, so if we're doing analysis there's
        # no point introducing them to the build.
        env['build_configdb'] = False
        env['build_lgpl'] = False
        env['build_apps'] = False
        env['build_libs'] = False
        env['build_images'] = False

# Override CXX if needed.
if env['iwyu']:
    # Make sure IWYU is fully freestanding and only refers to our headers.
    env['iwyu'] += ' -Xiwyu --no_default_mappings'
    env['iwyu'] += ' -Xiwyu --transitive_includes_only'

    env['CXXFLAGS'] = env['CFLAGS'] = []

    env['CC'] = env['iwyu']
    env['CXX'] = env['iwyu']

    # We don't want disk images when doing an IWYU run.
    env['build_images'] = False

# Save the cache, all the options are configured
if (not env['build_tests_only']) and env['cache']:
    opts.Save('options.cache', env)
    for k in autogen_opts.keys():
        autogen_env[k] = env[k]
    autogen_opts.Save('.autogen.cache', autogen_env)

# Make build messages much prettier.
exports = ['env', 'host_env']
misc.prettifyBuildMessages(env)
if not env['build_tests_only']:
    misc.prettifyBuildMessages(userspace_env)
    exports.append('userspace_env')
misc.prettifyBuildMessages(host_env)

# Generate custom builders and add to environment.
misc.generate(env)
if not env['build_tests_only']:
    misc.generate(userspace_env)
misc.generate(host_env)

VariantDir(env['BUILDDIR'], 'src', duplicate=0)
VariantDir(os.path.join(env['BUILDDIR'], 'external'), 'external', duplicate=0)

SConscript(os.path.join(env['BUILDDIR'], 'SConscript'), exports=exports)

subarch_dump = env.get('SUBARCH_DIR', '')
if subarch_dump:
    subarch_dump = ' (+%s)' % subarch_dump

print()
print("**** This Pedigree build (r%s, %s%s + %s, by %s) begins at %s ****" %
        (env['PEDIGREE_REVISION'], env['ARCH_DIR'], subarch_dump,
            env['MACH_TARGET'], env['PEDIGREE_USER'], datetime.today()))
print()

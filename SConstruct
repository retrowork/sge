#############################################################################
# $Id$

import os
import re
from .tools.scripts.Walk import Walk


#############################################################################

platform = str(Platform())

if not platform in ['win32', 'cygwin', 'posix']:
   print 'Unsupported platform', platform
   Exit(1)
   
   
#############################################################################
#
# CLASS: SGEEnvironment
#

class SGEEnvironment(Environment):

   def __init__(self,**kw):
      Environment.__init__(self,**kw)
      self.debug = 1
      self.shared = 0
      self.m_buildRoot = os.getcwd()
      self.m_libs = []
      self.m_libPaths = []
      self.m_incPaths = []
      self.m_defines = []
      if self.get('unicode'):
         self.Append(CPPDEFINES=['_UNICODE', 'UNICODE'])
      else:
         self.Append(CPPDEFINES=['_MBCS'])
      if self.get('shared'):
         self.SetShared()
      else:   
         self.SetStatic()

   def GetBuildDir(self):
      buildDir = 'build' + os.sep + str(Platform()) + os.sep
      if self.IsDebug():
         buildDir += 'debug-'
      else:
         buildDir += 'release-'
      if self.IsShared():
         buildDir += 'shared-'
      else:
         buildDir += 'static-'
      if self.get('unicode'):
         buildDir += 'unicode'
      else:
         buildDir += 'ansi'
      return buildDir

   def UseJpeg(self):
      self.m_libs += ['jpeg']
      self.m_libPaths += [MakeLibPath('jpeg')]
      self.m_incPaths += ['#3rdparty/jpeg']
      
   def UseTinyxml(self):
      self.m_libs += ['tinyxml']
      self.m_libPaths += [MakeLibPath('tinyxml')]
      self.m_defines += ['TIXML_USE_STL']
      self.m_incPaths += ['#3rdparty/tinyxml']
      
   def UseCg(self):
      self.m_libs += ['Cg', 'CgGL']
      self.m_libPaths += ['#3rdparty/Cg/lib']
      self.m_incPaths += ['#3rdparty/Cg/include']
      
   def UseGL(self):
      if platform == 'win32':
         self.Append(LIBS    = ['opengl32', 'glu32'])
      elif platform in ['cygwin', 'posix']:
         self.Append(CPPPATH = ['/usr/include', '/usr/X11R6/include'])
         self.Append(LIBPATH = ['/usr/lib', '/usr/X11R6/lib'])
         self.Append(LIBS    = ['Xi', 'GLU', 'GL', 'Xext', 'X11'])
      self.m_libs += ['glew']
      self.m_libPaths += [MakeLibPath('glew')]
      self.m_incPaths += ['#3rdparty/glew/include']
      self.m_defines += ['GLEW_STATIC']
         
   def UseZLib(self):
      self.m_libs += ['zlibwapi']
      self.m_libPaths += [MakeLibPath('zlib')]
      self.m_incPaths += ['#3rdparty/zlib', '#3rdparty/zlib/contrib/minizip']
      
   def UseLua(self):
      self.m_libs += ['lua']
      self.m_libPaths += [MakeLibPath('lua')]
      self.m_incPaths += ['#3rdparty/lua/src']
      
   def UseNvTriStrip(self):
      self.m_libs += ['NvTriStrip']
      self.m_libPaths += [MakeLibPath('NvTriStrip')]
      self.m_incPaths += ['#3rdparty/NvTriStrip/include']
      
   def UseUnitTestPP(self):
      self.Append(CPPDEFINES=['HAVE_UNITTESTPP'])
      self.m_libs += ['UnitTest++']
      self.m_libPaths += [MakeLibPath('UnitTest++')]
      self.m_incPaths += ['#3rdparty/UnitTest++/src']
      
   def UseFreetype(self):
      self.m_libs += ['freetype']
      self.m_libPaths += [MakeLibPath('freetype')]
      self.m_incPaths += ['#3rdparty/freetype/include']
      
   def UseFTGL(self):
      self.UseFreetype()
      self.UseGL()
      self.m_libs += ['ftgl']
      self.m_libPaths += [MakeLibPath('ftgl')]
      self.m_incPaths += ['#3rdparty/ftgl/include']
      if self.IsShared():
         self.Append(CPPDEFINES=['FTGL_DLL_EXPORTS', 'FTGL_LIBRARY'])
      else:
         self.Append(CPPDEFINES=['FTGL_LIBRARY_STATIC'])
      
   def SetCommon(self):
      self.Append(CPPPATH=['#api'])
      if platform == 'win32':
         self.Append(CPPDEFINES=['WIN32', '_WIN32'])
         self.Append(CCFLAGS=['/EHsc'])
         # HACK: turn off deprecation warnings from MSVC 2005 compiler
         if self.get('MSVS_VERSION') == '8.0':
            self.Append(CPPDEFINES=['_CRT_SECURE_NO_DEPRECATE'])
      elif platform == 'cygwin':
         self.Append(CCFLAGS=['-mwindows'])
            
   def IsShared(self):
      return self.shared
         
   def SetStatic(self):
      self.shared = 0
      self.Append(CPPDEFINES=['NO_AUTO_EXPORTS'])
      if platform == 'win32':
         if self.IsDebug():
            self.Append(CCFLAGS=['/MTd'])
         else:
            self.Append(CCFLAGS=['/MT'])
      
   def SetShared(self):
      self.shared = 1
      if platform == 'win32':
         if self.IsDebug():
            self.Append(CCFLAGS=['/MDd'])
         else:
            self.Append(CCFLAGS=['/MD'])
            
   def IsDebug(self):
      return self.debug
      
   def SetDebug(self):
      self.debug = 1
      self.SetCommon()
      if platform == 'win32':
         self.Append(CCFLAGS=['/Od'])
         if self.get('MSVS_VERSION') == '8.0':
            self.Append(CCFLAGS=['/RTC1'])
         else:
            self.Append(CCFLAGS=['/GZ'])
      elif platform in ['cygwin', 'posix']:
         self.Append(CCFLAGS=['-ggdb'])
         
   def SetRelease(self):
      self.debug = 0
      self.SetCommon()
      if platform == 'win32':
         self.Append(CCFLAGS=['/O2'],
                     CPPDEFINES=['NDEBUG'],
                     LINKFLAGS=['/OPT:REF'])
      elif platform in ['cygwin', 'posix']:
         self.Append(CCFLAGS=['-o3'])
         
   def __PreBuild(self, *args, **kw):
      if 'include_path' in kw:
         self.Append(CPPPATH=kw.pop('include_path'))
      if 'lib_path' in kw:
         libPaths = map(MakeLibPath, kw.pop('lib_path'))
         self.Append(LIBPATH=libPaths)
      if 'libs' in kw:
         self.Append(LIBS=kw.pop('libs'))
      self.Append(LIBS = self.m_libs)
      self.Append(LIBPATH = self.m_libPaths)
      self.Append(CPPPATH = self.m_incPaths)
      self.Append(CPPDEFINES = self.m_defines)

   def BuildStaticLibrary(self, *args, **kw):
      if 'lib_path' in kw:
         kw.pop('lib_path')
      if 'libs' in kw:
         kw.pop('libs')
      self.__PreBuild(*args, **kw)
      self.StaticLibrary(*args, **kw)

   def BuildSharedLibrary(self, *args, **kw):
      self.__PreBuild(*args, **kw)
      # add def file to sources
      if 'deffile' in kw:
         if 'source' in kw:
            kw['source'].append(kw.pop('deffile'))
         else:
            args[1].append(kw.pop('deffile'))
      if self['PLATFORM'] == 'win32':
         self.Append(LIBS=['user32', 'kernel32', 'gdi32', 'winmm'])
      if 'target' in kw:
         target = kw['target']
      else:
         target = args[0]
      if 'export_symbol' in kw:
         export_symbol = kw['export_symbol']
      else:
         export_symbol = target.upper() + '_EXPORTS'
      self.Append(CPPDEFINES=[export_symbol])
      self.SharedLibrary(*args, **kw)

   def BuildLibrary(self, *args, **kw):
      if self.IsShared():
         self.BuildSharedLibrary(*args, **kw)
      else:
         self.BuildStaticLibrary(*args, **kw)
         
   def BuildExecutable(self, *args, **kw):
      self.__PreBuild(*args, **kw)
      if 'target' in kw:
         target = kw['target']
      else:
         target = args[0]
      if self['PLATFORM'] == 'win32':
         self.Append(LIBS=['user32', 'kernel32', 'gdi32', 'winmm'])
         if self.get('debug'):
            self['PDB'] = target + '.pdb'
      self.Program(*args, **kw)


#############################################################################

opts = Options(None, ARGUMENTS)
opts.AddOptions(
   BoolOption('debug', 'Build with debugging enabled', 0),
   BoolOption('unicode', 'Build with _UNICODE defined', 0),
   BoolOption('shared', 'Build shared libraries', 0))

env = SGEEnvironment(ENV = os.environ, options = opts)

if env.get('debug'):
   env.SetDebug()
else:
   env.SetRelease()
   
buildRootDir = env.GetBuildDir()
Export('buildRootDir')
if not os.path.isdir(buildRootDir):
   os.makedirs(buildRootDir)

def MakeLibPath(path):
   return '#' + os.path.join(buildRootDir, path.lstrip('#'))

Help("Usage: scons [debug] [unicode]" + opts.GenerateHelpText(env))

########################################

SConsignFile(os.path.join(buildRootDir, '.sconsign'))

sconscripts = Walk(os.getcwd(), 1, 'SConscript', 0)

for script in sconscripts:

   # The target name is the name of the directory in which
   # the SConscript file resides
   temp = script.split(os.sep)
   target = temp[-2]
   
   sourceDir = os.path.split(script)[0]
   buildDir = os.path.join(buildRootDir, target)
   buildScript = os.path.join(buildDir, temp[-1])

   BuildDir(buildDir, sourceDir, duplicate=0)
   SConscript(buildScript, exports='env')
   env.Alias(target, buildDir)

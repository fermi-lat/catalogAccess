import glob,os,platform

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

catalogAccessLib = libEnv.StaticLibrary('catalogAccess', listFiles(['src/*.cxx']))

progEnv.Tool('catalogAccessLib')
main_testBin = progEnv.Program('main_test', listFiles(['src/test/main_test.cxx']))

progEnv.Tool('registerObjects', package = 'catalogAccess', libraries = [catalogAccessLib], testApps = [main_testBin], includes = listFiles(['catalogAccess/*.h']))
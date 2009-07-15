# -*- python -*-
# $Id: SConscript,v 1.6 2009/07/14 17:58:00 jrb Exp $
# Authors: Aymeric Sauvageon <asauvageon@cea.fr>
# Version: catalogAccess-00-04-02

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('catalogAccessLib', depsOnly = 1)
catalogAccessLib = libEnv.StaticLibrary('catalogAccess', listFiles(['src/*.cxx']))

progEnv.Tool('catalogAccessLib')
main_testBin = progEnv.Program('main_test', 'src/test/main_test.cxx')
test_catalogAccess = progEnv.Program('test_catalogAccess', 'src/test/main_test.cxx')

#progEnv.Tool('registerObjects', package = 'catalogAccess', libraries = [catalogAccessLib], testApps = [main_testBin, test_catalogAccess],
#             includes = listFiles(['catalogAccess/*.h']), data = listFiles(['data/*'], recursive = True))

progEnv.Tool('registerTargets', package = 'catalogAccess',
             staticLibraryCxts = [[catalogAccessLib, libEnv]],
             testAppCxts = [[main_testBin, progEnv],
                            [test_catalogAccess, progEnv]],
             includes = listFiles(['catalogAccess/*.h']),
             data = listFiles(['data/*'], recursive = True))

def generate(env, **kw):
    if not kw.get('depsOnly',0):
        env.Tool('addLibrary', library = ['catalogAccess'], package = 'catalogAccess')
    env.Tool('st_facilitiesLib')
    env.Tool('tipLib')

def exists(env):
    return 1

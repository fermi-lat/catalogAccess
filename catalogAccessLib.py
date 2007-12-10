def generate(env, **kw):
    env.Tool('addLibrary', library = ['catalogAccess'], package = 'catalogAccess')
    env.Tool('st_facilitiesLib')
    env.Tool('tipLib')

def exists(env):
    return 1

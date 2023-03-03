import shutil
import os

Import("env")

def post_program_action(source, target, env):
    print('post program action called with:')
    print('%s' % source)
    print('-----------------')
    print('%s' % target)
    print('-----------------')
    print('%s' % env)
    print('-----------------')
    project_root = env['PROJECT_DIR']
    release_dir = os.path.join(project_root, 'release')
    program_path = target[0].get_abspath()  
    print('post program copying: %s -> %s' % (program_path, release_dir))
    shutil.copy(program_path, release_dir)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", post_program_action)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", post_program_action)

import os
import time

spirVExtension = '.spv'
vertexExtension = '.vert'
fragmentExtension = '.frag'
root = '.'

files = os.listdir(root)

fileInfo = {}

def compile(fn):
    print('file \'{}\':'.format(fn))
    print('>> compiling...')
    os.system('glslangValidator -V {} -o {}'.format(
        os.path.join(root, fn),
        os.path.join(root, fn + spirVExtension)
    ))
    print('>> done')

while True:
    for file in files:
        if file.find(spirVExtension) < 0 and \
            (file.find(vertexExtension) >= 0 or file.find(fragmentExtension) >= 0):
            if file not in fileInfo or os.path.getmtime(file) != fileInfo[file]:
                fileInfo[file] = os.path.getmtime(file)
                compile(file)
    print('All file processed. {}'.format(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())))
    time.sleep(2)


import random

pcount = [0] * 8

f = open('./instance.txt', 'w')

for i in range(0, 8):
    pcount[i] = random.randint(0, 127)
    f.write('p{} {}\n'.format(i, pcount[i]))

hlocal = '''LX 0.0 0.0 0.0 0.0 0.0 0.0 1.0 1.0 0.2\n'''

f.write(hlocal)

for i in range(0, 8):
    c = pcount[i]
    for j in range(0, c):
        f.write('L{} '.format(i))
        f.write('{} {} {} '.format(random.random() * 5.0 - 2.5, random.random() * 5.0 - 2.5, random.random() * 5.0 - 2.5))
        f.write('{} {} {} '.format(random.random() * 6.28, random.random() * 6.28, random.random() * 6.28))
        f.write('{} {} {} '.format(random.random() * 0.5 + 0.5, random.random() * 0.5 + 0.5, random.random() * 0.5 + 0.5))
        f.write('\n')

f.close()
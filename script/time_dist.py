from io import TextIOWrapper
import sys


dist = [100, 200, 300, 400, 500, 600]
time = []

train = int(sys.argv[2])
sensor = 70

cr = open(sys.argv[1], 'r')
log = open(f'log_train{train}.txt', 'w')
log.write(f'{train}\n')

def wait_int(f: TextIOWrapper):
    while True:
        str = f.readline().rstrip('\n\r ')
        # print(str)
        if len(str) > 0 and str.isdigit():
            return int(str)


def test_dist(dist):
    print(f'loc {train} {sensor} -{dist} f')
    print(f'tr {train} 26')
    start_time = wait_int(cr) * 10
    end_time = wait_int(cr) * 10
    print(f'tr {train} 0')
    t = end_time - start_time
    return t  # t in ms


for s in dist:
    print(f'start dist {s} testing?', end='', file=sys.stderr)
    input()
    time.append(test_dist(s))

dist_um = [s * 1000 for s in dist]

log.write(f'{time}\n')
log.write(f'{dist_um}\n')

print('done', file=sys.stderr)

cr.close()
log.close()

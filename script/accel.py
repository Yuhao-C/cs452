from io import TextIOWrapper
import sys


def wait_line(f: TextIOWrapper):
    while True:
        str = f.readline()
        if len(str) > 0:
            return str

def calc_decel(v_top: float, decel_dist: float):
    '''v_top: um / s, decel_dist: mm, decel: um / s^2'''
    decel = v_top * v_top / (decel_dist * 1000) / 2
    return decel

def accel():
    dist = 250000
    f = open(sys.argv[1], 'r')
    while True:
        start_time = int(wait_line(f)) * 10
        end_time = int(wait_line(f)) * 10
        t = end_time - start_time
        print(f'accel_time: {t} ms')

        decel_dist = int(input('decel_dist mm? '))
        # ms
        # um / ms^2
        accel = 2 * dist / (t ** 2)
        # um / ms
        v_top = 2 * dist / t
        # um / s^2
        decel = calc_decel(v_top * 1000, decel_dist)

        print(f'accel: {accel * 1000000} um/s^2, v_top: {v_top * 1000} um/s, decel: {decel} um/s^2\n')

if __name__ == '__main__':
    accel()

# um / ms^2
# acc = [(1, 0.0890), (24, 0.0843), (58, 0.0762), (74, 0.0653), (78, 0.0448), (79, 0.0937)]

# for id, a in acc:
#     # um / s^2
#     print(id, a * 1000000)

# train 79
# 0.0937 -> 93700

# train 24
# 0.0843 -> 84300

# train 78
# 0.0448 -> 44800

# train 58
# 0.0762 -> 76200

# train 1
# 0.0890 -> 89000

# train 74
# 0.0653 -> 65300
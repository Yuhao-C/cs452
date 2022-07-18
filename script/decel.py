def calc_decel(v_top: float, decel_dist: float):
    '''v_top: um / s, decel_dist: mm, decel: um / s^2'''
    decel = v_top * v_top / (decel_dist * 1000) / 2
    return decel

if __name__ == '__main__':
    while True:
        v_top = int(input('v_top? '))
        decel_dist = int(input('decel_dist? '))
        print(f'decel: {calc_decel(v_top, decel_dist)} um / s^2')
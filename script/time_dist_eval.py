from io import TextIOWrapper
import sys

import numpy


velo = {1: 329000, 2: 492000, 24: 342000, 58: 305000, 74: 478000, 78: 267500, 79: 372000}

log = open(sys.argv[1], 'r')

train = eval(log.readline())
time_ms = eval(log.readline())
dist_um = eval(log.readline())
log.close()

log = open(sys.argv[1], 'a')

coeff = numpy.polyfit(time_ms, dist_um, 2)
deriv_coeff = [coeff[0] * 2, coeff[1]]

log.write(f'coeff: {coeff}\n')
log.write(f'deriv_coeff: {deriv_coeff}\n')

v = velo[train] / 1000  # v in um/ms
accel_finish_time = (v - coeff[1]) / (2 * coeff[0])
log.write(f'accel finish time: {accel_finish_time} ms\n')

t = accel_finish_time
accel_finish_dist = coeff[0] * t * t + coeff[1] * t + coeff[2]
log.write(f'accel finish dist: {accel_finish_dist} um\n')

print('done')

log.close()
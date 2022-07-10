#!/usr/bin/python3.9

import json
from io import TextIOWrapper
import sys


def train_set(f: TextIOWrapper, i: int, field: str, val):
    f.write("  trains[{}].{} = {};\n".format(i, field, val))


def calc_decel(dist: int, velocity: int) -> int:
    return round(velocity * velocity / dist / 2)


def calc_accel_dist(accel: int, velocity: int) -> int:
    return round(velocity * velocity / accel / 2)


def generate_src(in_file: str, file: str):
    data = None
    with open(in_file, "r") as f:
        data = json.load(f)
    with open(file, "w") as f:
        f.write("#ifndef CALIBRATION_TRAIN_DATA_H_\n")
        f.write("#define CALIBRATION_TRAIN_DATA_H_\n\n")
        f.write('#include "marklin/train.h"\n\n')
        f.write("void initTrains(marklin::Train *trains) {\n")
        for i, train in enumerate(data["trains"]):
            train_set(f, i, "id", train["id"])
            train_set(f, i, "velocity", train["velocity"])
            train_set(f, i, "accelSlow", train["accelSlow"])
            train_set(f, i, "decelSlow", train["decelSlow"])
            train_set(f, i, "accelDist", train["accelDist"])
            train_set(f, i, "accelDelay", train["accelDelay"])
            train_set(f, i, "stopDist", train["stopDist"])
            f.write("\n")
        f.write("}\n\n")
        f.write("#endif  // CALIBRATION_TRAIN_DATA_H_\n")


if __name__ == "__main__":
    generate_src(sys.argv[1], sys.argv[2])

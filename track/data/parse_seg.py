import yaml
import sys


def sensor_index(name: str) -> int:
    g = ord(name[0]) - ord("A")
    i = int(name[1:]) - 1
    return g * 16 + i


def get_reverse_sensor(index: int) -> int:
    return index + 1 if index % 2 == 0 else index - 1


def branch_index(name: str) -> int:
    num = int(name[2:]) - 1
    return 80 + num * 2


in_files = sys.argv[1:3]
out_file = sys.argv[3]
func_names = ["initSegA", "initSegB"]
track_sizes = [144, 140]

file = open(out_file, "w")

file.write('#include "../include/track_data.h"\n\n')

for in_file, func_name, track_size in zip(in_files, func_names, track_sizes):
    data = None
    with open(in_file) as f:
        data = yaml.safe_load(f)

    seg_enter_count = [0 for _ in range(37)]
    seg_leave_count = [0 for _ in range(37)]

    file.write(f"void {func_name}(track_node *track) {{\n")
    file.write(f"  for (int i = 0; i < {track_size}; ++i) {{\n")
    file.write("    track[i].enterSeg[0] = -1;\n")
    file.write("    track[i].enterSeg[1] = -1;\n")
    file.write("    track[i].leaveSeg[0] = -1;\n")
    file.write("    track[i].leaveSeg[1] = -1;\n")
    file.write("  }\n\n")

    for i, item in enumerate(data[:80]):
        sensor = sensor_index(item["id"])
        assert i == sensor
        enter = item["enter"]
        leave = item["leave"]
        assert enter[0] == data[get_reverse_sensor(i)]["leave"][0]
        for index, e in enumerate(enter):
            if e != -1:
                file.write(f"  track[{i}].enterSeg[{index}] = {e};\n")
                seg_enter_count[e] += 1

        file.write(f"  track[{i}].leaveSeg[0] = {leave[0]};\n")
        seg_leave_count[leave[0]] += 1
        if leave[0] != leave[1] and leave[1] != -1:
            file.write(f"  track[{i}].leaveSeg[1] = {leave[1]};\n")
            seg_leave_count[leave[1]] += 1

    for item in data[80:]:
        node_num = branch_index(item["id"])
        enter = item["enter"]
        leave = item["leave"]
        for index, e in enumerate(enter):
            if e != -1:
                file.write(f"  track[{node_num}].enterSeg[{index}] = {e};\n")
                seg_enter_count[e] += 1

    file.write("}\n\n")

    print([(i, c) for i, c in enumerate(seg_enter_count)])
    print([(i, c) for i, c in enumerate(seg_leave_count)])
    assert all(map(lambda e, l: e == l, seg_enter_count, seg_leave_count))

file.close()

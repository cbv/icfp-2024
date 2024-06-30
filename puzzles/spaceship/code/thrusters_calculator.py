import sys

def main(start_location, end_location, start_velo, end_velo, min_steps, max_steps):
    data_dicts = construct_dicts(max_steps)
    thruster_dict = data_dicts[0]
    distance_dict = data_dicts[1]
    return get_thrusters(start_location, end_location, start_velo, end_velo, min_steps, max_steps, thruster_dict)

def get_thrusters(start_location, end_location, start_velo, end_velo, min_steps, max_steps, thruster_dict):
    rslt = dict()
    delta_velo = end_velo - start_velo
    for steps in range(min_steps, max_steps + 1):
        delta_location = end_location - start_location - start_velo * steps
        if delta_location in thruster_dict.keys():
            if delta_velo in thruster_dict[delta_location].keys():
                if steps in thruster_dict[delta_location][delta_velo].keys():
                    rslt[steps] = thruster_dict[delta_location][delta_velo][steps]
    return rslt

def construct_dicts(max_steps):
    #distance_dict[end_location][time_steps] = {possible_velos}
    distance_dict = dict()
    distance_dict[0] = dict()
    distance_dict[0][0] = set([0])

    #thruster_dict[offset][end_velo][time_steps] = thruster string
    thruster_dict = dict()
    thruster_dict[0] = dict()
    thruster_dict[0][0] = dict()
    thruster_dict[0][0][0] = ""
    velo_shift_code = {1:"+", 0:"0", -1:"-"}

    for time_step in range(max_steps):
        for dist in distance_dict.copy().keys():
            for exit_velo in distance_dict[dist][time_step]:
                for velo_shift in range(-1, 2):
                    dist_end = dist + exit_velo + velo_shift
                    if  dist_end not in distance_dict.keys():
                        distance_dict[dist_end] = dict()
                    if dist_end not in thruster_dict.keys():
                        thruster_dict[dist_end] = dict()
                    if time_step + 1 not in distance_dict[dist_end].keys():
                        distance_dict[dist_end][time_step + 1] = set()
                    distance_dict[dist_end][time_step + 1].add(exit_velo + velo_shift)
                    thruster_code_prev = thruster_dict[dist][exit_velo][time_step]
                    thruster_code = thruster_code_prev + velo_shift_code[velo_shift]
                    if exit_velo + velo_shift not in thruster_dict[dist_end]:
                        thruster_dict[dist_end][exit_velo + velo_shift] = dict()
                    thruster_dict[dist_end][exit_velo + velo_shift][time_step + 1] = thruster_code
    return (thruster_dict, distance_dict)

if __name__ == "__main__":
    start_location = int(sys.argv[1])
    end_location = int(sys.argv[2])
    start_velo = int(sys.argv[3])
    end_velo = int(sys.argv[4])
    min_steps = int(sys.argv[5])
    max_steps = int(sys.argv[6])
    result = main(start_location, end_location, start_velo, end_velo, min_steps, max_steps)
    sys.stdout.write(str(result))
    sys.exit()
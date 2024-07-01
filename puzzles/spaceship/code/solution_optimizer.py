import os
import math

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
    velo_shift_code = {-1:"4", 0:"5", 1:"6"}

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

thruster_dict, _ = construct_dicts(100)

def make_squares(coordinates):
    squares = []
    for i in range(int(len(coordinates)/2)):
        squares.append((int(coordinates[2*i]), int(coordinates[2*i + 1])))
    return(squares)

def high_triangle(max_value, count):
    return (max_value + (max_value - count + 1)) * count / 2

def low_triangle(count):
    return (count + 1) * count/2

def fastest_path(x_start, vx_start, x_end, vx_end):
    delta_x = x_end - x_start
    delta_vx = vx_end - vx_start
    time_steps = abs(delta_vx)
    while True:
        extra_both_directions = math.floor((time_steps - abs(delta_vx)) / 2) 
        if delta_vx > 0:
            accel_count = abs(delta_vx) + extra_both_directions
            decel_count = extra_both_directions
        else:
            accel_count = extra_both_directions
            decel_count = abs(delta_vx) + extra_both_directions
        dist_adjustment = vx_start * time_steps
        max_x = high_triangle(time_steps, accel_count) - low_triangle(decel_count) + dist_adjustment
        min_x = low_triangle(accel_count) - high_triangle(time_steps, decel_count) + dist_adjustment
        if (min_x <= delta_x and delta_x <= max_x):
            return time_steps
        time_steps += 1

def spaceship_tracker(movement, x = 0, y = 0, vx = 0, vy = 0):
    results = []
    results.append((x, y, vx, vy))
    for key_press in movement:
        if key_press in ('7', '8', '9'):
            vy += 1
        if key_press in ('1', '2', '3'):
            vy -= 1
        if key_press in ('3', '6', '9'):
            vx += 1
        if key_press in ('1', '4', '7'):
            vx -= 1
        x += vx
        y += vy
        results.append((x, y, vx, vy))
    return results

def decomp_thrust(thrusts):
    y_decomp = thrusts
    x_decomp = thrusts
    for i in range(1, 10):
        y_decomp = y_decomp.replace(str(i), str(3 * ((i - 1) // 3) + 2))
        x_decomp = x_decomp.replace(str(i), str(4 + ((i - 1) % 3)))
    return((x_decomp, y_decomp))

def combine_thrusts(x_thrusts, y_thrusts):
    x_offsets = [(int(x_thrust) - 5)  for x_thrust in x_thrusts]
    combined = [str(int(y_thrusts[i]) + x_offsets[i]) for i in range(len(x_thrusts))]
    return "".join(combined)

def swap_x_to_y(thrust_code):
    thrust_code = thrust_code.replace("4", "2")
    thrust_code = thrust_code.replace("6", "8")
    return thrust_code

summary_file = open(r'solutions\spaceship\late_solutions\summary.txt', 'w')
for counter in range(1,26):
    print(counter)

    f = open(r'solutions\spaceship\spaceship' + str(counter) +'.txt', 'r')
    data = f.read()
    f.close()

    f = open(r'puzzles\spaceship\spaceship' + str(counter) + '.txt', 'r')
    coords_str = f.read()
    coords_array = coords_str.split()
    target_squares = set(make_squares(coords_array))
    f.close()

    solution = data.split(' ')[-1]
    ship_log = spaceship_tracker(solution)
    unseen_target_squares = target_squares.copy()
    target_idxs = set()
    for i in range(len(ship_log)):
        if ship_log[i][:2] in unseen_target_squares:
            unseen_target_squares.remove(ship_log[i][:2])
            target_idxs.add(i)

    square_visit_idxs = [0] + sorted(target_idxs)
    square_visit_order = [ship_log[i][:2] for i in square_visit_idxs]
    square_visit_times = [square_visit_idxs[i + 1] - square_visit_idxs[i] for i in range(len(target_squares))]

    targets_with_velocities = [ship_log[i] for i in square_visit_idxs]
    optimal_times_x = [None] * len(target_squares)
    optimal_times_y = [None] * len(target_squares)
    optimal_times = [None] * len(target_squares)
    thrust_segments = [None] * len(target_squares)
    for i in range(len(target_squares)):
        optimal_times_x[i] = fastest_path(targets_with_velocities[i][0], targets_with_velocities[i][2], targets_with_velocities[i + 1][0], targets_with_velocities[i + 1][2])
        optimal_times_y[i] = fastest_path(targets_with_velocities[i][1], targets_with_velocities[i][3], targets_with_velocities[i + 1][1], targets_with_velocities[i + 1][3])
        optimal_times[i] = max(optimal_times_x[i], optimal_times_y[i])
        xd, yd = decomp_thrust(solution[square_visit_idxs[i]:square_visit_idxs[i + 1]])
        time_steps = optimal_times[i]
        state_init = ship_log[square_visit_idxs[i]]
        state_end = ship_log[square_visit_idxs[i + 1]]
        x_init, y_init, vx_init, vy_init = state_init
        x_end, y_end, vx_end, vy_end = state_end
        adjusted_x_movement = x_end - x_init - (time_steps * vx_init)
        adjusted_y_movement = y_end - y_init - (time_steps * vy_init)
        processed = False
        if square_visit_times[i] != optimal_times[i]:
            if ((adjusted_x_movement in thruster_dict) and \
            vx_end - vx_init in thruster_dict[adjusted_x_movement] and \
            (time_steps in thruster_dict[adjusted_x_movement][vx_end - vx_init])):
                if ((adjusted_y_movement in thruster_dict) and \
                vy_end - vy_init in thruster_dict[adjusted_y_movement] and \
                (time_steps in thruster_dict[adjusted_y_movement][vy_end - vy_init])):
                    x_thrust_code = thruster_dict[adjusted_x_movement][vx_end - vx_init][time_steps]
                    y_thrust_code = swap_x_to_y(thruster_dict[adjusted_y_movement][vy_end - vy_init][time_steps])
                    thrust_segments[i] = combine_thrusts(x_thrust_code, y_thrust_code)
                    processed = True
        # default to previous solution if unknown
        if not processed:
            thrust_segments[i] = solution[square_visit_idxs[i]:square_visit_idxs[i + 1]]
   

    solution_new = "".join(thrust_segments)
    if len(solution_new) < len(solution):
        log_new = spaceship_tracker(solution_new)
        squares_new = {info[:2] for info in log_new}
        if set(target_squares).difference(squares_new):
                raise ValueError('new solution fails')

        late_file = open(r'solutions\spaceship\late_solutions\spaceship' + str(counter) + '.txt', 'w')
        _ = late_file.write('solve spaceship{} {}'.format(counter, solution_new))
        late_file.close()

        _ = summary_file.write('Update solution for spaceship{}\n'.format(counter))
        _ = summary_file.write('old solution length: {}\n'.format(len(solution)))
        _ = summary_file.write('new solution_length: {}\n'.format(len(solution_new)))
        _ = summary_file.write('savings: {}\n\n'.format(len(solution) - len(solution_new)))
summary_file.close()
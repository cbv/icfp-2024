import sys
import math

def main(file):
    f = open(file, 'r')
    coords_str = f.read()
    coords_array = coords_str.split()
    target_squares = set(make_squares(coords_array))
    starting_point = (0, 0, 0, 0, ())
    try:
        spaceship_inputs = get_all_segments(starting_point, target_squares)
        visited_squares = spaceship_tracker(spaceship_inputs)
        if not all([square in visited_squares for square in target_squares]):
            raise AssertionError('Spaceship Movement does not visit all targets!')
        return spaceship_inputs
    except:
        return "Not Found"

def get_all_segments(starting_point, target_squares):
    rslt = ""
    target_squares = target_squares.difference((starting_point[0], starting_point[1]))
    while len(target_squares) > 1:
        next_segment = get_next_segment(starting_point, target_squares)
        old_point = next_segment[0]
        for point in next_segment[1:]:
            dx = point[0] - old_point[0]
            dy = point[1] - old_point[1]
            vx = old_point[2]
            vy = old_point[3]
            rslt += str(5 + (dx - vx) + 3 * (dy - vy))
            old_point = point
        target_squares.remove((point[0], point[1]))
        starting_point = (*point, ())
    next_segment = get_next_segment(starting_point, target_squares, True)
    old_point = next_segment[0]
    for point in next_segment[1:]:
        dx = point[0] - old_point[0]
        dy = point[1] - old_point[1]
        vx = old_point[2]
        vy = old_point[3]
        rslt += str(5 + (dx - vx) + 3 * (dy - vy))
        old_point = point
    return rslt

def make_squares(coordinates):
    squares = []
    for i in range(int(len(coordinates)/2)):
        squares.append((int(coordinates[2*i]), int(coordinates[2*i + 1])))
    return(squares)

def spaceship_tracker(movement):
    results = []
    x = 0
    vx = 0
    y = 0
    vy = 0
    results.append((0,0))
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
        results.append((x, y))
    return results

def get_next_segment(starting_point, target_squares, last = False):
    points_to_process = set([starting_point])
    points_seen = set([starting_point])
    still_processing = True
    prev_lookup = dict()
    rounds = 0
    while still_processing:
        rounds += 1
        if (rounds > 12):
            raise ValueError('taking too long!')
        for starting_point in points_to_process.copy():
            next_point = None
            points_to_process.remove(starting_point)
            x = starting_point[0]
            y = starting_point[1]
            vx_start = starting_point[2]
            vy_start = starting_point[3]
            for vx_new in range(vx_start - 1, vx_start + 2):
                for vy_new in range(vy_start - 1, vy_start + 2):
                    target_squares_seen = starting_point[4]
                    x_new = x + vx_new
                    y_new = y + vy_new
                    if (x_new, y_new) in target_squares:
                        if target_squares_seen == ():
                            need_backtrack = last
                            target_squares_seen = (x_new, y_new, vx_new, vy_new)
                            target_squares_seen = (*target_squares_seen, (*target_squares_seen, ()))
                            prev_lookup[target_squares_seen] = starting_point
                        else:
                            need_backtrack = True
                        if need_backtrack:
                            if last or (target_squares_seen[0] != x_new) or (target_squares_seen[1] != y_new):
                                late_point = target_squares_seen
                                backtrack = []
                                done = False
                                while not done:
                                    if late_point in prev_lookup:
                                        new_point = prev_lookup[late_point]
                                        backtrack.append(late_point[:4])
                                        late_point = new_point
                                    else:
                                        backtrack.append(late_point[:4])
                                        #raise ValueError('Test')
                                        return backtrack[::-1]
                    new_point = (x + vx_new, y + vy_new, vx_new, vy_new, target_squares_seen)
                    if new_point not in points_seen:
                        points_to_process.add(new_point)
                        points_seen.add(new_point)
                        prev_lookup[new_point] = (starting_point)

if __name__ == "__main__":
    file_name = sys.argv[1]
    sys.stdout.write(file_name + ": " + main(file_name) + "\n")
    sys.exit()
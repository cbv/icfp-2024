import sys
import math

coords_str = "0 1 0 1 -1 2 -1 4"

def main(file):
    f = open(file, 'r')
    coords_str = f.read()
    coords_array = coords_str.split()
    target_squares = make_squares(coords_array)
    spaceship_inputs = calculate_inputs(target_squares)
    visited_squares = spaceship_tracker(spaceship_inputs)
    if not all([square in visited_squares for square in target_squares]):
        raise AssertionError('Spaceship Movement does not visit all targets!')
    return calculate_inputs(target_squares)

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

def get_opp_direction(direction):
    if direction == "2":
        return "8"
    if direction == "4":
        return "6"
    if direction == "6":
        return "4"
    if direction == "8":
        return "2"
    raise ValueError("Invalid direction!")

def get_fast_distance(distance, direction):
    result = ""
    accel_time = math.floor(math.sqrt(distance))
    result = direction * accel_time
    excess = distance - accel_time**2
    opp_direction = get_opp_direction(direction)
    speed = accel_time
    while speed > 0:
        if (excess >= speed):
            result = result + "5"
            excess = excess - speed
        else:
            result = result + opp_direction
            speed -= 1
    return(result)

def combine_inputs(x_inputs, y_inputs):
    result = ""
    combined_length = min(len(x_inputs), len(y_inputs))
    for i in range(combined_length):
        if (x_inputs[i] == "4"):
            if (y_inputs[i] == "8"):
                result += "7"
            elif (y_inputs[i] == "5"):
                result += "4"
            elif (y_inputs[i] == "2"):
                result += "1"
        elif (x_inputs[i] == "5"):
            result += y_inputs[i]
        elif (x_inputs[i] == "6"):
            if (y_inputs[i] == "8"):
                result += "9"
            elif (y_inputs[i] == "5"):
                result += "6"
            elif (y_inputs[i] == "2"):
                result += "3"
    if (len(x_inputs) > len(y_inputs)):
        result += x_inputs[combined_length:]
    elif (len(y_inputs) > len(x_inputs)):
        result += y_inputs[combined_length:]
    return result

def calculate_inputs(target_squares):
    result = ""
    x = 0
    y = 0
    for square in target_squares:
        x_offset = square[0] - x
        y_offset = square[1] - y
        if (x_offset == 0):
            x_inputs = ""
        else:
            if (x_offset < 0):
                direction = "4"
            else:
                direction = "6"
            distance = abs(x_offset)
            x_inputs = get_fast_distance(distance, direction)
        if (y_offset == 0):
            y_inputs = ""
        else:
            if (y_offset < 0):
                direction = "2"
            else:
                direction = "8"
            distance = abs(y_offset)
            y_inputs = get_fast_distance(distance, direction)
        result += combine_inputs(x_inputs, y_inputs)
        x = square[0]
        y = square[1]
    return(result)

if __name__ == "__main__":
    file_name = sys.argv[1]
    sys.exit(main(file_name))
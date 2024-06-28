import sys

def main(file):
    f = open(file, 'r')
    coords_str = f.read()
    coords_array = coords_str.split()
    target_squares = make_squares(coords_array)
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

# start_movement = '236659'
# spaceship_tracker(start_movement)

def calculate_inputs(target_squares):
    result = ""
    x = 0
    y = 0
    for square in target_squares:
        x_offset = square[0] - x
        y_offset = square[1] - y
        if (x_offset != 0):
            if (x_offset < 0):
                result = result + "4"
                if (abs(x_offset) > 1):
                    result = result + "5" * (abs(x_offset) - 1)
                result = result + "6"
            if (x_offset > 0):
                result = result + "6"
                if (abs(x_offset) > 1):
                    result = result + "5" * (abs(x_offset) - 1)
                result = result + "4"
        if (y_offset != 0):
            if (y_offset < 0):
                result = result + "2"
                if (abs(y_offset) > 1):
                    result = result + "5" * (abs(y_offset) - 1)
                result = result + "8"
            if (y_offset > 0):
                result = result + "8"
                if (abs(x_offset) > 1):
                    result = result + "5" * (abs(x_offset) - 1)
                result = result + "2"
        x = square[0]
        y = square[1]
    return(result)

if __name__ == "__main__":
    file_name = sys.argv[1]
    sys.exit(main(file_name))
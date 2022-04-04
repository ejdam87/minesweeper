#include <stdbool.h>
#include "minesweeper.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>   // For random numbers

#define E_BITS 8
#define LOWER_4_BITS 15
#define BIT_6 32
#define BIT_5 16

#define UNUSED(A)(void)(A)

/* ************************************************************** *
 *                         HELP FUNCTIONS                         *
 * ************************************************************** */

// Cell is 16bits -- upper 8 is val -- lower 4 is number of neighbour mins
// -- 5th is -> revealed/non-revealed
// 6th --> flag or not flagged

// Getters
char get_val(uint16_t cell) {
    return (char)(cell >> E_BITS);
}

char get_neighbour_mins(uint16_t cell) {
    return (char)(cell & LOWER_4_BITS);
}

bool is_flag(uint16_t cell) {
    return cell & BIT_6;
}

bool is_mine(uint16_t cell) {
    return get_val(cell) == 'M';
}

bool is_revealed(uint16_t cell) {
    return cell & BIT_5;
}

int get_number(uint16_t cell) {
    if (get_val(cell) == 'M') {
        return 0;
    }
    return get_neighbour_mins(cell);
}


/* ************************************************************** *
 *                         INPUT FUNCTIONS                        *
 * ************************************************************** */

bool is_valid(int ch) {
    return ch == 'X' || ch == 'M' || ch == 'F' || ch == 'W' || ch == '.' || (ch >= '0' && ch <= '8');
}

bool set_cell(uint16_t * cell, char val) {

    if (cell == NULL) {
        return false;
    }

    int upper = toupper(val);

    if (!is_valid(upper)) {
        return false;
    }

    *cell = 0;
    switch (upper) {

        case 'F':
            *cell = 'M';
            *cell = *cell << E_BITS;
            *cell |= BIT_6;
            break;
        case 'M':
            *cell = 'M';
            *cell = *cell << E_BITS;
            break;
        case 'X':
            *cell = 'X';
            *cell = *cell << E_BITS;
            break;
        case 'W':
            *cell = '.';
            *cell = *cell << E_BITS;
            *cell |= BIT_6;
            break;
        case '.':
            *cell = '.';
            *cell = *cell << E_BITS;
            *cell |= BIT_5; // revealed
            break;
        default: {
            if (upper >= '0' && upper <= '8') {
                *cell = upper;
                *cell = *cell << E_BITS;
                *cell |= BIT_5; // revealed
            }
            break;
        }
    }

    return true;
}

int load_board(size_t rows, size_t cols, uint16_t board[rows][cols]) {
    size_t chars_to_load = rows * cols;
    int ch = getchar();
    bool status;
    size_t row = 0;
    size_t col = 0;

    while (chars_to_load > 0 && ch != EOF) {

        if (!is_valid(toupper(ch))) {
            ch = getchar();
            continue;
        }

        status = set_cell( & board[row][col], (char) ch);

        col += 1;
        if (col >= cols) {
            col = 0;
            row += 1;
        }

        ch = getchar();

        if (status) {
            chars_to_load -= 1;
        }

    }

    if (chars_to_load != 0) {
        return -1;
    }

    return postprocess(rows, cols, board);
}

bool valid_pos(size_t rows, size_t cols,
               size_t i, size_t j) {

    // >= 0 is implicit (size_t)
    return i < rows && j < cols;
}

uint8_t get_mins(size_t rows, size_t cols,
                 size_t i, size_t j,
                 uint16_t board[rows][cols]) {

    int min_count = 0;
    for (int drow = -1; drow <= 1; drow++) {
        for (int dcol = -1; dcol <= 1; dcol++) {

            if (drow == 0 && dcol == 0) {
                continue;
            }
            if (!valid_pos(rows, cols, drow + i, dcol + j)) {
                continue;
            }

            if (get_val(board[i + drow][j + dcol]) == 'M') {
                min_count += 1;
            }
        }
    }

    return min_count;
}

int postprocess(size_t rows, size_t cols, uint16_t board[rows][cols]) {

    // Out of bounds
    if (rows < MIN_SIZE || rows > MAX_SIZE ||
        cols < MIN_SIZE || cols > MAX_SIZE) {
        return -1;
    }

    // Mine in the corner
    if (is_mine(board[0][0]) || is_mine(board[rows - 1][0]) ||
        is_mine(board[0][cols - 1]) || is_mine(board[rows - 1][cols - 1])) {

        return -1;
    }

    // No mines + numbers good
    int mine_count = 0;
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {

            if (is_mine(board[i][j])) {
                mine_count += 1;
            }

            uint8_t real_min_count = get_mins(rows, cols, i, j, board);

            if (is_revealed(board[i][j]) && isdigit(get_val(board[i][j]))) {
                // Invalid mins number
                if (get_val(board[i][j]) - '0' != real_min_count) {
                    return -1;
                }
            }

            board[i][j] |= real_min_count; // setting lower 4 bits with mine count
        }
    }

    if (mine_count == 0) {
        return -1;
    }

    return mine_count;
}

/* ************************************************************** *
 *                        OUTPUT FUNCTIONS                        *
 * ************************************************************** */

char show_cell(uint16_t cell) {
    char val = (char) get_val(cell);

    if (is_mine(cell) && is_revealed(cell)) {
        return 'M';
    }

    if (is_revealed(cell)) {
        if (get_neighbour_mins(cell) == 0) {
            return ' ';
        }
        return (char)(get_neighbour_mins(cell) + '0');
    }

    if (is_flag(cell)) {
        return 'F';
    }

    if (!is_revealed(cell)) {
        return 'X';
    }

    if (isdigit(val)) {
        return val;
    }

    if (val == '.') {
        return ' ';
    }

    return val;

}

void print_line(size_t cols) {
    for (size_t i = 0; i < cols; i++) {
        putchar('+');

        for (int j = 0; j < 3; j++) {
            putchar('-');
        }
    }
    putchar('+');
    putchar('\n');
}

void print_vals(size_t rows, size_t cols, size_t current_row, uint16_t board[rows][cols]) {
    char to_print;
    for (size_t i = 0; i < cols; i++) {
        putchar('|');

        to_print = show_cell(board[current_row][i]);

        switch (to_print) {
            case 'M':
            case ' ':
                putchar(' ');
                putchar(to_print);
                putchar(' ');
                break;
            case 'F':
                printf("_F_");
                break;
            case 'X':
                printf("XXX");
                break;
            default:
                break;
        }

        if (isdigit(to_print)) {
            putchar(' ');
            putchar(to_print);
            putchar(' ');
        }
    }

    putchar('|');
    putchar('\n');
}

int print_board(size_t rows, size_t cols, uint16_t board[rows][cols]) {
    // Horizontal numbering
    printf("    ");
    for (size_t n = 0; n < cols; n++) {

        if (n <= 9) {
            putchar(' ');
        }
        printf("%lu", n);
        if (n <= 9 || (n >= 10 && n <= 99)) {
            putchar(' ');
        }

        if (n != cols - 1) {
            putchar(' ');
        }
    }

    putchar('\n');

    for (size_t i = 0; i < rows; i++) {

        printf("   ");
        print_line(cols);

        if (i <= 9) {
            putchar(' ');
        }
        printf("%lu", i);

        if (i <= 9 || (i >= 10 && i <= 99)) {
            putchar(' ');
        }
        print_vals(rows, cols, i, board);
    }

    printf("   ");
    print_line(cols);

    return 0;
}

/* ************************************************************** *
 *                    GAME MECHANIC FUNCTIONS                     *
 * ************************************************************** */

int reveal_cell(size_t rows, size_t cols, uint16_t board[rows][cols], size_t row, size_t col) {

    // --- Invalid
    if (row >= rows) {
        return -1;
    }
    if (col >= cols) {
        return -1;
    }

    if (is_revealed(board[row][col]) || is_flag(board[row][col])) {
        return -1;
    }
    // ---

    if (get_number(board[row][col]) != 0 || is_mine(board[row][col])) {
        return reveal_single( & board[row][col]);
    }

    reveal_floodfill(rows, cols, board, row, col);
    return 0;

}

int reveal_single(uint16_t *cell) {

    if (cell == NULL){
        return -1;
    }

    if (is_revealed(*cell) || is_flag(*cell)) {
        return -1;
    }

    if (is_mine(*cell)) {
        *cell |= BIT_5; // set on reveal flag
        return 1;
    }

    *cell |= 16;
    return 0;

}

void reveal_floodfill(size_t rows, size_t cols, uint16_t board[rows][cols], size_t row, size_t col) {

    for (int drow = -1; drow <= 1; drow++) {
        for (int dcol = -1; dcol <= 1; dcol++) {

            if (dcol == 0 && drow == 0) {
                continue;
            }
            if (!valid_pos(rows, cols, row + drow, col + dcol)) { // Out of bounds
                continue;
            }
            if (is_revealed(board[drow + row][dcol + col])) {
                continue;
            }

            if (get_number(board[drow + row][dcol + col]) != 0) { // Found cell with neighbour mine

                if (is_flag(board[drow + row][dcol + col])){
                    board[drow + row][dcol + col] |= BIT_5;
                }
                else{
                    reveal_single(&board[drow + row][dcol + col]);
                }

            } else {

                if (is_flag(board[drow + row][dcol + col])){
                    board[drow + row][dcol + col] |= BIT_5;
                }
                else{
                    reveal_single(&board[drow + row][dcol + col]);
                }
                reveal_floodfill(rows, cols, board, row + drow, col + dcol);
            }
        }
    }
}

int flag_cell(size_t rows, size_t cols, uint16_t board[rows][cols], size_t row, size_t col) {

    if (is_revealed(board[row][col])) {
        return INT16_MIN;
    }

    if (is_flag(board[row][col])) {
        board[row][col] &= ~(BIT_6);
    } else {
        board[row][col] |= BIT_6;
    }

    // this used to be separate function (Valgrind was making issues)
    int mins = 0;
    int flags = 0;

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            if (is_mine(board[i][j])) {
                mins += 1;
            }

            if (is_flag(board[i][j])) {
                flags += 1;
            }

        }
    }
    return mins - flags;

}

bool is_solved(size_t rows, size_t cols, uint16_t board[rows][cols]) {
    int unrevealed = 0;
    int mins = 0;

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {

            if (!is_revealed(board[i][j])) {
                unrevealed += 1;
            }
            if (is_mine(board[i][j])) {
                mins += 1;
            }
        }
    }

    return mins == unrevealed;

}

/* ************************************************************** *
 *                         BONUS FUNCTIONS                        *
 * ************************************************************** */

bool mine_already_planted(size_t mines, size_t index,
                          size_t mine_rows[mines], size_t mine_cols[mines],
                          size_t new_row, size_t new_col){

    for (size_t i = 0; i < index; i++){

        if (mine_rows[i] == new_row && mine_cols[i] == new_col){
            return true;
        }
    }

    return false;

}

int generate_random_board(size_t rows, size_t cols, uint16_t board[rows][cols], size_t mines) {
    srand((unsigned) time(NULL) * 5); // Initialize seed

    // Initialize board
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            set_cell( & board[i][j], 'X');
        }
    }

    size_t mine_rows[mines];
    size_t mine_cols[mines];

    // Get mine positions
    size_t index = 0;
    while (index < mines) {

        size_t new_row = rand() % rows;
        size_t new_col = rand() % cols;

        if (mine_already_planted(mines, index, mine_rows, mine_cols, new_row, new_col)){
            continue;
        }

        // Mine in the corner
        if (new_row == 0 && new_col == 0) {
            continue;
        }
        if (new_row == rows - 1 && new_col == 0) {
            continue;
        }
        if (new_row == rows - 1 && new_col == cols - 1) {
            continue;
        }
        if (new_row == 0 && new_col == cols - 1) {
            continue;
        }


        mine_rows[index] = new_row;
        mine_cols[index] = new_col;
        index += 1;

    }


    // Plant mines
    for (size_t i = 0; i < mines; i++) {
        set_cell( & board[mine_rows[i]][mine_cols[i]], 'M');
    }

    // The postprocess function should be called at the end of the generate random board function
    return postprocess(rows, cols, board);
}

int find_mines(size_t rows, size_t cols, uint16_t board[rows][cols]) {
    // TODO: Implement me
    UNUSED(rows);
    UNUSED(cols);
    UNUSED(board);
    return -1;
}

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define VIEWPORT_WIDTH 80   // Default width of the visible portion of the game world
#define VIEWPORT_HEIGHT 80  // Default height of the visible portion of the game world

// Player and enemy structures for holding stats
typedef struct {
    char name[20];
    int health;
    int attack;
    int defense;
    int x; // Goblin's x position
    int y; // Goblin's y position
} Character;

// Function prototypes
void display_game_world(char world[MAX_HEIGHT][MAX_WIDTH], int rows, int player_x, int player_y, int *row_lengths);
void move_player(char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y, char move, int rows, int *row_lengths);
void move_goblins(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int rows, int *row_lengths);
void enable_raw_mode();
void disable_raw_mode();
void display_info_section(Character player, Character goblin, const char *event_log);
Character find_nearby_goblin(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int rows, int *row_lengths);
void start_turn_based_combat(Character *player, Character *goblin, char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y);
bool is_adjacent(int x1, int y1, int x2, int y2);
bool has_line_of_sight(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int goblin_x, int goblin_y);

// Struct to store original terminal settings so we can restore them later
struct termios orig_termios;

int main() {
    char world[MAX_HEIGHT][MAX_WIDTH];  // Buffer to hold the game world
    int player_x = -1, player_y = -1;   // Player's position
    int rows = 0;
    int row_lengths[MAX_HEIGHT];  // To track the length of each row
    char filename[] = "gameworld.txt";
    
    // Open and read the game world from a file
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }

    // Read the game world from the file
    while (fgets(world[rows], MAX_WIDTH, file) != NULL) {
        row_lengths[rows] = strlen(world[rows]) - 1; // Store the length of each row, excluding newline
        for (int col = 0; col < row_lengths[rows]; col++) {
            if (world[rows][col] == 'P') {
                player_x = rows;
                player_y = col;
            }
        }
        rows++;
    }

    fclose(file);

    // Player setup
    Character player = {"Hero", 100, 20, 5, player_x, player_y};
    char event_log[100] = "Player exploring the world.";

    // Enable raw mode for instant key presses
    enable_raw_mode();

    // Game loop
    char move;
    while (1) {
        // Display the game world with the viewport
        display_game_world(world, rows, player_x, player_y, row_lengths);

        // Find goblin near the player, if any
        Character nearby_goblin = find_nearby_goblin(world, player_x, player_y, rows, row_lengths);

        // Display player and goblin stats, and event log
        display_info_section(player, nearby_goblin, event_log);

        // Get player input for movement without pressing enter
        move = getchar();

        // Quit if 'q' is pressed
        if (move == 'q') {
            break;
        }

        // Move the player only if not in combat mode
        if (strcmp(nearby_goblin.name, "Goblin") != 0 || !is_adjacent(player_x, player_y, nearby_goblin.x, nearby_goblin.y)) {
            move_player(world, &player_x, &player_y, move, rows, row_lengths);
        }

        // If a goblin is adjacent to the player, start a turn-based combat mode
        if (is_adjacent(player_x, player_y, nearby_goblin.x, nearby_goblin.y)) {
            start_turn_based_combat(&player, &nearby_goblin, world, &player_x, &player_y);
            if (player.health <= 0) {
                printf("Player was defeated!\n");
                break;
            }
            if (nearby_goblin.health <= 0) {
                snprintf(event_log, sizeof(event_log), "Goblin defeated!");
                world[nearby_goblin.x][nearby_goblin.y] = ' ';  // Remove goblin from world
            }
        }

        // Move goblins toward the player, but only if they have line-of-sight
        move_goblins(world, player_x, player_y, rows, row_lengths);
    }

    // Restore the original terminal settings before exiting
    disable_raw_mode();

    return 0;
}

// Function to display the game world with a viewport centered on the player
void display_game_world(char world[MAX_HEIGHT][MAX_WIDTH], int rows, int player_x, int player_y, int *row_lengths) {
    system("clear");  // Clears the terminal for a cleaner output (Linux-specific)
    printf("Game World Viewport:\n");

    int start_row = player_x - VIEWPORT_HEIGHT / 2;
    int start_col = player_y - VIEWPORT_WIDTH / 2;

    if (start_row < 0) start_row = 0;
    if (start_col < 0) start_col = 0;

    for (int i = 0; i < VIEWPORT_HEIGHT && (start_row + i) < rows; i++) {
        for (int j = 0; j < VIEWPORT_WIDTH && (start_col + j) < row_lengths[start_row + i]; j++) {
            printf("%c", world[start_row + i][start_col + j]);
        }
        printf("\n");
    }
}

// Function to move the player within the game world
void move_player(char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y, char move, int rows, int *row_lengths) {
    int new_x = *player_x;
    int new_y = *player_y;

    // Calculate new position based on input
    if (move == 'w' && *player_x > 0) {
        new_x--;
    } else if (move == 's' && *player_x < rows - 1) {
        new_x++;
    } else if (move == 'a' && *player_y > 0) {
        new_y--;
    } else if (move == 'd' && *player_y < row_lengths[*player_x] - 1) {
        new_y++;
    }

    // Check if the new position is not a wall ('#') or a goblin ('G')
    if (world[new_x][new_y] == ' ' || world[new_x][new_y] == 'G') {
        // Update the game world with the new player position
        world[*player_x][*player_y] = ' ';  // Clear the old position
        world[new_x][new_y] = 'P';          // Move the player to the new position
        *player_x = new_x;
        *player_y = new_y;
    }
}

// Function to move goblins toward the player if they have line-of-sight
void move_goblins(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int rows, int *row_lengths) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < row_lengths[i]; j++) {
            if (world[i][j] == 'G') {
                // Goblin AI: Move towards the player if they have line-of-sight
                if (!has_line_of_sight(world, player_x, player_y, i, j)) {
                    continue;
                }

                int move_x = i, move_y = j;
                if (i < player_x) move_x++;
                else if (i > player_x) move_x--;
                if (j < player_y) move_y++;
                else if (j > player_y) move_y--;

                if (world[move_x][move_y] == ' ') {
                    world[i][j] = ' ';  // Clear old goblin position
                    world[move_x][move_y] = 'G';  // Move goblin to new position
                }
            }
        }
    }
}

// Function to check if the goblin has line-of-sight to the player using Bresenham's algorithm
bool has_line_of_sight(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int goblin_x, int goblin_y) {
    int dx = abs(player_x - goblin_x);
    int dy = abs(player_y - goblin_y);
    int sx = (goblin_x < player_x) ? 1 : -1;
    int sy = (goblin_y < player_y) ? 1 : -1;
    int err = dx - dy;

    while (goblin_x != player_x || goblin_y != player_y) {
        if (world[goblin_x][goblin_y] == '#') {
            return false;  // There's a wall between goblin and player
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            goblin_x += sx;
        }
        if (e2 < dx) {
            err += dx;
            goblin_y += sy;
        }
    }

    return true;  // No walls found in the line-of-sight
}

// Function to display the info section with player/goblin stats and event log
void display_info_section(Character player, Character goblin, const char *event_log) {
    printf("\n--- Info Section ---\n");
    printf("Player: %s | HP: %d | ATK: %d | DEF: %d\n", player.name, player.health, player.attack, player.defense);

    // Only display goblin info if a goblin is near
    if (strcmp(goblin.name, "Goblin") == 0) {
        printf("Goblin: %s | HP: %d | ATK: %d | DEF: %d\n", goblin.name, goblin.health, goblin.attack, goblin.defense);
    }

    printf("Event: %s\n", event_log);
}

// Function to find a nearby goblin within 1 tile of the player
Character find_nearby_goblin(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int rows, int *row_lengths) {
    Character goblin = {"None", 0, 0, 0};  // Default: no goblin nearby

    // Check the 1-tile vicinity around the player for a goblin ('G')
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int check_x = player_x + dx;
            int check_y = player_y + dy;

            if (check_x >= 0 && check_x < rows && check_y >= 0 && check_y < row_lengths[check_x]) {
                if (world[check_x][check_y] == 'G') {
                    goblin = (Character){"Goblin", 50, 10, 2, check_x, check_y};  // Goblin stats and position
                    return goblin;
                }
            }
        }
    }

    return goblin;
}

// Function to start a turn-based combat between player and goblin
void start_turn_based_combat(Character *player, Character *goblin, char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y) {
    printf("\n--- Combat Mode ---\n");
    char move;
    while (player->health > 0 && goblin->health > 0) {
        // Display combat options
        printf("Player: HP = %d, Goblin: HP = %d\n", player->health, goblin->health);
        printf("Choose action: (a)ttack, (m)ove: ");
        move = getchar();

        // Player chooses to attack
        if (move == 'a') {
            printf("Player attacks!\n");
            goblin->health -= (player->attack - goblin->defense);
            if (goblin->health <= 0) {
                printf("Goblin is defeated!\n");
                return;
            }

            // Goblin's turn to attack
            printf("Goblin attacks!\n");
            player->health -= (goblin->attack - player->defense);
            if (player->health <= 0) {
                printf("Player is defeated!\n");
                return;
            }
        }

        // Player chooses to move
        else if (move == 'm') {
            printf("Move direction (w/a/s/d): ");
            move = getchar();
            move_player(world, player_x, player_y, move, MAX_HEIGHT, MAX_WIDTH);
            // If the player moves out of adjacency, exit combat mode
            if (!is_adjacent(*player_x, *player_y, goblin->x, goblin->y)) {
                printf("Player moved out of combat range.\n");
                return;
            }
        }
    }
}

// Helper function to check if two points are adjacent
bool is_adjacent(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) <= 1 && abs(y1 - y2) <= 1;
}

// Function to enable raw mode (for instant keypresses)
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);  // Get current terminal attributes
    struct termios raw = orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON);  // Disable echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);  // Apply the new settings
}

// Function to disable raw mode and restore original terminal settings
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);  // Restore original settings
}


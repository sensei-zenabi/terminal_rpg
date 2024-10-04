#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define VIEWPORT_WIDTH 20   // Width of the visible portion of the game world
#define VIEWPORT_HEIGHT 10  // Height of the visible portion of the game world

// Player and enemy structures for holding stats
typedef struct {
    char name[20];
    int health;
    int attack;
    int defense;
} Character;

// Function prototypes
void display_game_world(char world[MAX_HEIGHT][MAX_WIDTH], int rows, int player_x, int player_y, int *row_lengths);
void move_player(char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y, char move, int rows, int *row_lengths);
void enable_raw_mode();
void disable_raw_mode();
void display_info_section(Character player, Character enemy, const char *event_log);

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

    // Player and enemy setup
    Character player = {"Hero", 100, 20, 5};
    Character enemy = {"Goblin", 50, 10, 2};
    char event_log[100] = "Player encountered a Goblin!";

    // Enable raw mode for instant key presses
    enable_raw_mode();

    // Game loop
    char move;
    while (1) {
        // Display the game world with the viewport
        display_game_world(world, rows, player_x, player_y, row_lengths);
        
        // Display player and enemy stats, and event log
        display_info_section(player, enemy, event_log);

        // Get player input for movement without pressing enter
        move = getchar();

        // Quit if 'q' is pressed
        if (move == 'q') {
            break;
        }

        // Move the player
        move_player(world, &player_x, &player_y, move, rows, row_lengths);

        // Update event log based on actions (example)
        if (move == 'w' || move == 'a' || move == 's' || move == 'd') {
            snprintf(event_log, sizeof(event_log), "Player moved to (%d, %d)", player_x, player_y);
        }
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

    // Check if the new position is not a wall ('#')
    if (world[new_x][new_y] == ' ') {
        // Update the game world with the new player position
        world[*player_x][*player_y] = ' ';  // Clear the old position
        world[new_x][new_y] = 'P';          // Move the player to the new position
        *player_x = new_x;
        *player_y = new_y;
    }
}

// Function to display the info section with player/enemy stats and event log
void display_info_section(Character player, Character enemy, const char *event_log) {
    printf("\n--- Info Section ---\n");
    printf("Player: %s | HP: %d | ATK: %d | DEF: %d\n", player.name, player.health, player.attack, player.defense);
    printf("Enemy: %s | HP: %d | ATK: %d | DEF: %d\n", enemy.name, enemy.health, enemy.attack, enemy.defense);
    printf("Event: %s\n", event_log);
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


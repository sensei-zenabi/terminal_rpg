#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define VIEWPORT_WIDTH 40  // Width of the visible portion of the game world
#define VIEWPORT_HEIGHT 20 // Height of the visible portion of the game world
#define MAX_GOBLINS 10     // Maximum number of goblins to spawn

// Player and enemy structures for holding stats
typedef struct {
    char name[20];
    int health;
    int attack;
    int defense;
    int x; // Character's x position
    int y; // Character's y position
} Character;

// Function prototypes
void display_game_world(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y);
void move_player(char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y, char move);
void move_goblins(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, Character goblins[], int goblin_count);
void enable_raw_mode();
void disable_raw_mode();
void display_info_section(Character player, Character goblins[], int goblin_count, const char *event_log);
void start_turn_based_combat(Character *player, Character *goblin);
bool is_adjacent(int x1, int y1, int x2, int y2);
bool has_line_of_sight(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, int goblin_x, int goblin_y);
void save_game_state(char world[MAX_HEIGHT][MAX_WIDTH], Character player, Character goblins[], int goblin_count, const char *filename);
bool load_game_state(char world[MAX_HEIGHT][MAX_WIDTH], Character *player, Character goblins[], int *goblin_count, const char *filename);
void generate_procedural_dungeon(char world[MAX_HEIGHT][MAX_WIDTH], Character *player, Character goblins[], int *goblin_count);
void connect_rooms_with_corridor(char world[MAX_HEIGHT][MAX_WIDTH], int x1, int y1, int x2, int y2);

// Struct to store original terminal settings so we can restore them later
struct termios orig_termios;

int main() {
    char world[MAX_HEIGHT][MAX_WIDTH];  // Buffer to hold the game world
    Character player = {"Hero", 100, 20, 5, 0, 0};  // Player stats
    Character goblins[MAX_GOBLINS];  // Array to hold goblin stats
    int goblin_count = 0;  // Number of goblins

    char filename[] = "saved_game.txt";
    char event_log[100] = "Player exploring the world.";
    
    // Prompt to load game or start a new game
    printf("Start a new game (n) or load saved game (l)? ");
    char choice = getchar();
    getchar();  // Consume newline
    if (choice == 'l') {
        if (!load_game_state(world, &player, goblins, &goblin_count, filename)) {
            printf("No saved game found, starting new game...\n");
            generate_procedural_dungeon(world, &player, goblins, &goblin_count);
        }
    } else {
        generate_procedural_dungeon(world, &player, goblins, &goblin_count);
    }

    // Enable raw mode for instant key presses
    enable_raw_mode();

    // Game loop
    char move;
    while (1) {
        // Display the game world with the viewport
        display_game_world(world, player.x, player.y);

        // Display player and goblin stats, and event log
        display_info_section(player, goblins, goblin_count, event_log);

        // Get player input for movement or action without pressing enter
        move = getchar();

        // Quit if 'q' is pressed
        if (move == 'q') {
            // Save the game state when quitting
            save_game_state(world, player, goblins, goblin_count, filename);
            break;
        }

        // Move the player
        move_player(world, &player.x, &player.y, move);

        // Check for nearby goblins and initiate combat
        for (int i = 0; i < goblin_count; i++) {
            if (is_adjacent(player.x, player.y, goblins[i].x, goblins[i].y)) {
                start_turn_based_combat(&player, &goblins[i]);
                if (player.health <= 0) {
                    printf("Player was defeated!\n");
                    return 0;  // End the game
                }
                if (goblins[i].health <= 0) {
                    // Remove the goblin from the world
                    world[goblins[i].x][goblins[i].y] = ' ';
                }
            }
        }

        // Move goblins toward the player
        move_goblins(world, player.x, player.y, goblins, goblin_count);
    }

    // Restore the original terminal settings before exiting
    disable_raw_mode();

    return 0;
}

// Function to generate a procedural dungeon-like game world
void generate_procedural_dungeon(char world[MAX_HEIGHT][MAX_WIDTH], Character *player, Character goblins[], int *goblin_count) {
    // Initialize the world to walls
    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            world[i][j] = '#';
        }
    }

    // Randomly generate rooms and corridors
    srand(time(NULL));
    int room_count = 5 + rand() % 5;  // Random number of rooms (5 to 10)
    int rooms[room_count][4];  // Stores x, y, width, and height of each room

    for (int r = 0; r < room_count; r++) {
        int room_width = 5 + rand() % 6;   // Random width (5 to 10)
        int room_height = 5 + rand() % 6;  // Random height (5 to 10)
        int room_x = rand() % (MAX_HEIGHT - room_height);
        int room_y = rand() % (MAX_WIDTH - room_width);

        rooms[r][0] = room_x;
        rooms[r][1] = room_y;
        rooms[r][2] = room_width;
        rooms[r][3] = room_height;

        // Create the room
        for (int i = 0; i < room_height; i++) {
            for (int j = 0; j < room_width; j++) {
                world[room_x + i][room_y + j] = ' ';  // Empty space for the room
            }
        }

        // Randomly place player in the first room
        if (r == 0) {
            player->x = room_x + room_height / 2;
            player->y = room_y + room_width / 2;
            world[player->x][player->y] = 'P';
        }

        // Randomly place goblins in other rooms
        if (r > 0 && *goblin_count < MAX_GOBLINS) {
            int goblin_x = room_x + rand() % room_height;
            int goblin_y = room_y + rand() % room_width;
            goblins[*goblin_count] = (Character){"Goblin", 50, 10, 2, goblin_x, goblin_y};
            world[goblin_x][goblin_y] = 'G';
            (*goblin_count)++;
        }
    }

    // Connect rooms with corridors
    for (int r = 1; r < room_count; r++) {
        int x1 = rooms[r - 1][0] + rooms[r - 1][2] / 2;
        int y1 = rooms[r - 1][1] + rooms[r - 1][3] / 2;
        int x2 = rooms[r][0] + rooms[r][2] / 2;
        int y2 = rooms[r][1] + rooms[r][3] / 2;
        connect_rooms_with_corridor(world, x1, y1, x2, y2);
    }
}

// Function to connect two rooms with a corridor
void connect_rooms_with_corridor(char world[MAX_HEIGHT][MAX_WIDTH], int x1, int y1, int x2, int y2) {
    // Create a horizontal corridor
    if (y1 < y2) {
        for (int y = y1; y <= y2; y++) {
            world[x1][y] = ' ';
        }
    } else {
        for (int y = y2; y <= y1; y++) {
            world[x1][y] = ' ';
        }
    }

    // Create a vertical corridor
    if (x1 < x2) {
        for (int x = x1; x <= x2; x++) {
            world[x][y2] = ' ';
        }
    } else {
        for (int x = x2; x <= x1; x++) {
            world[x][y2] = ' ';
        }
    }
}

// Function to display the game world with a viewport centered on the player
void display_game_world(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y) {
    system("clear");  // Clears the terminal for a cleaner output (Linux-specific)
    printf("Game World Viewport:\n");

    // Calculate the starting row and column of the viewport
    int start_row = player_x - VIEWPORT_HEIGHT / 2;
    int start_col = player_y - VIEWPORT_WIDTH / 2;

    // Keep the viewport within bounds
    if (start_row < 0) start_row = 0;
    if (start_col < 0) start_col = 0;
    if (start_row + VIEWPORT_HEIGHT >= MAX_HEIGHT) start_row = MAX_HEIGHT - VIEWPORT_HEIGHT;
    if (start_col + VIEWPORT_WIDTH >= MAX_WIDTH) start_col = MAX_WIDTH - VIEWPORT_WIDTH;

    // Display the visible portion of the game world
    for (int i = 0; i < VIEWPORT_HEIGHT; i++) {
        for (int j = 0; j < VIEWPORT_WIDTH; j++) {
            printf("%c", world[start_row + i][start_col + j]);
        }
        printf("\n");
    }
}

// Function to move the player within the game world
void move_player(char world[MAX_HEIGHT][MAX_WIDTH], int *player_x, int *player_y, char move) {
    int new_x = *player_x;
    int new_y = *player_y;

    // Calculate new position based on input
    if (move == 'w' && *player_x > 0) {
        new_x--;
    } else if (move == 's' && *player_x < MAX_HEIGHT - 1) {
        new_x++;
    } else if (move == 'a' && *player_y > 0) {
        new_y--;
    } else if (move == 'd' && *player_y < MAX_WIDTH - 1) {
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

// Function to move goblins toward the player, respecting line of sight
void move_goblins(char world[MAX_HEIGHT][MAX_WIDTH], int player_x, int player_y, Character goblins[], int goblin_count) {
    for (int i = 0; i < goblin_count; i++) {
        if (goblins[i].health > 0) {  // Only move goblins that are alive
            int move_x = goblins[i].x;
            int move_y = goblins[i].y;

            if (has_line_of_sight(world, player_x, player_y, goblins[i].x, goblins[i].y)) {
                if (goblins[i].x < player_x) move_x++;
                else if (goblins[i].x > player_x) move_x--;

                if (goblins[i].y < player_y) move_y++;
                else if (goblins[i].y > player_y) move_y--;

                // Check if the goblin can move to the new position
                if (world[move_x][move_y] == ' ') {
                    // Update the game world with the goblin's new position
                    world[goblins[i].x][goblins[i].y] = ' ';  // Clear the old position
                    goblins[i].x = move_x;
                    goblins[i].y = move_y;
                    world[move_x][move_y] = 'G';  // Move the goblin to the new position
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
void display_info_section(Character player, Character goblins[], int goblin_count, const char *event_log) {
    printf("\n--- Info Section ---\n");
    printf("Player: %s | HP: %d | ATK: %d | DEF: %d\n", player.name, player.health, player.attack, player.defense);

    // Display goblin info if any goblins are nearby
    for (int i = 0; i < goblin_count; i++) {
        if (goblins[i].health > 0) {
            printf("Goblin %d: HP = %d | ATK = %d | DEF = %d\n", i + 1, goblins[i].health, goblins[i].attack, goblins[i].defense);
        }
    }

    printf("Event: %s\n", event_log);
}

// Function to start turn-based combat between the player and a goblin
void start_turn_based_combat(Character *player, Character *goblin) {
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
            // In combat, moving will end the turn and allow goblin to attack
            printf("Goblin attacks as you move!\n");
            player->health -= (goblin->attack - player->defense);
            return;
        }
    }
}

// Helper function to check if two points are adjacent
bool is_adjacent(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) <= 1 && abs(y1 - y2) <= 1;
}

// Function to save the game state to a file
void save_game_state(char world[MAX_HEIGHT][MAX_WIDTH], Character player, Character goblins[], int goblin_count, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Unable to save the game.\n");
        return;
    }

    // Save the game world
    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            fputc(world[i][j], file);
        }
        fputc('\n', file);
    }

    // Save player stats
    fprintf(file, "Player %d %d %d %d %d\n", player.health, player.attack, player.defense, player.x, player.y);

    // Save goblin stats
    for (int i = 0; i < goblin_count; i++) {
        fprintf(file, "Goblin %d %d %d %d %d\n", goblins[i].health, goblins[i].attack, goblins[i].defense, goblins[i].x, goblins[i].y);
    }

    fclose(file);
    printf("Game saved successfully.\n");
}

// Function to load the game state from a file
bool load_game_state(char world[MAX_HEIGHT][MAX_WIDTH], Character *player, Character goblins[], int *goblin_count, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return false;  // No saved game file found
    }

    // Load the game world
    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            world[i][j] = fgetc(file);
        }
        fgetc(file);  // Consume newline
    }

    // Load player stats
    fscanf(file, "Player %d %d %d %d %d\n", &player->health, &player->attack, &player->defense, &player->x, &player->y);

    // Load goblin stats
    *goblin_count = 0;
    while (fscanf(file, "Goblin %d %d %d %d %d\n", &goblins[*goblin_count].health, &goblins[*goblin_count].attack, &goblins[*goblin_count].defense, &goblins[*goblin_count].x, &goblins[*goblin_count].y) == 5) {
        (*goblin_count)++;
    }

    fclose(file);
    printf("Game loaded successfully.\n");
    return true;
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


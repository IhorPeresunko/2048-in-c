#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/termios.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define EXIT_KEY 'q'
#define RESTART_KEY 'r'
#define LEFT_KEY 68
#define RIGHT_KEY 67
#define DOWN_KEY 66
#define UP_KEY 65
#define ARROW_KEY 27
#define ARROW_KEY_OPT 91

#define D_LEFT 1
#define D_RIGHT 2
#define D_DOWN 3
#define D_UP 4

#define GRID_SIZE 4
#define CELL_SIZE 4

#define GAME_WON 1
#define GAME_LOST 2
#define GAME_PROCESS 3

#define BLOCK_SPAWN_VALUE 2
#define BLOCK_WINNING_VALUE 2048

struct termios oldt, newt;

struct game_struct
{
  int grid[GRID_SIZE][GRID_SIZE];
  int score;
  int state;
} game;

void init();

int get_game_state()
{
  int lost = 1;

  for (int i = 0; i < GRID_SIZE; ++i)
  {
    for (int j = 0; j < GRID_SIZE; ++j)
    {
      if (game.grid[i][j] == BLOCK_WINNING_VALUE)
      {
        return GAME_WON;
      }

      if ((i > 0 && game.grid[i][j] == game.grid[i - 1][j]) ||
          (i < GRID_SIZE - 1 && game.grid[i][j] == game.grid[i + 1][j]) ||
          (j < GRID_SIZE - 1 && game.grid[i][j] == game.grid[i][j + 1]) ||
          (j > 0 && game.grid[i][j] == game.grid[i][j - 1]) ||
          (game.grid[i][j] == 0))
      {
        lost = 0;
      }
    }
  }

  if (lost)
  {
    return GAME_LOST;
  }

  return GAME_PROCESS;
}

void print_line()
{
  printf("+");
  for (int i = 0; i < GRID_SIZE * CELL_SIZE; ++i)
  {
    printf("-");
  }
  printf("+\n");
}

void clear_console()
{
  printf("\033[2J\033[H");
  printf("\n");
}

void print_grid()
{
  for (int i = 0; i < GRID_SIZE; ++i)
  {
    printf("|");

    for (int j = 0; j < GRID_SIZE; ++j)
    {
      for (int k = 0; k < CELL_SIZE; ++k)
      {
        if ((game.grid[i][j] > 1000 && k == 0) ||
            (game.grid[i][j] > 100 && k == 1) ||
            (game.grid[i][j] > 10 && k == 2) ||
            (game.grid[i][j] > 0 && k == 3))
        {
          printf("%d", game.grid[i][j]);
          break;
        }
        else
        {
          printf(" ");
        }
      }
    }

    printf("|\n");
  }
}

void draw()
{
  clear_console();

  printf("q - close | r - restart\n\n");

  printf("Score: %d\n", game.score);
  print_line();
  print_grid();
  print_line();

  usleep(30000);
}

void spawn_block()
{
  int empty_spots[GRID_SIZE * GRID_SIZE][2];
  int index = 0;

  for (int i = 0; i < GRID_SIZE; ++i)
  {
    for (int j = 0; j < GRID_SIZE; ++j)
    {
      if (game.grid[i][j] == 0)
      {
        empty_spots[index][0] = i;
        empty_spots[index][1] = j;
        ++index;
      }
    }
  }

  if (!index)
  {
    return;
  }

  int rand_index = rand() % index;

  int i = empty_spots[rand_index][0];
  int j = empty_spots[rand_index][1];
  game.grid[i][j] = BLOCK_SPAWN_VALUE;
}

void move_horizontal(int start, int end, int d)
{
  int changed = 0;

  for (int k = 0; k < GRID_SIZE; ++k)
  {
    for (int i = 0; i < GRID_SIZE; ++i)
    {
      for (int j = start; (start > end) ? j > end : j < end; j += d)
      {
        if (game.grid[i][j] != 0 && game.grid[i][j + d] == 0)
        {
          changed = 1;
          game.grid[i][j + d] = game.grid[i][j];
          game.grid[i][j] = 0;
          j += d;
        }
      }
    }
    if (!changed)
    {
      break;
    }
    else
    {
      draw();
    }
  }
}

void move_vertical(int start, int end, int d)
{
  int changed = 0;

  for (int k = 0; k < GRID_SIZE; ++k)
  {
    for (int j = 0; j < GRID_SIZE; ++j)
    {
      for (int i = start; (start > end) ? i > end : i < end; i += d)
      {
        if (game.grid[i][j] != 0 && game.grid[i + d][j] == 0)
        {
          changed = 1;
          game.grid[i + d][j] = game.grid[i][j];
          game.grid[i][j] = 0;
          i += d;
        }
      }
    }
    if (!changed)
    {
      break;
    }
    else
    {
      draw();
    }
  }
}

void move(int direction)
{
  switch (direction)
  {
  case D_RIGHT:
    move_horizontal(0, GRID_SIZE - 1, 1);
    break;
  case D_LEFT:
    move_horizontal(GRID_SIZE - 1, 0, -1);
    break;
  case D_DOWN:
    move_vertical(0, GRID_SIZE - 1, 1);
    break;
  case D_UP:
    move_vertical(GRID_SIZE - 1, 0, -1);
    break;

  default:
    break;
  }
}

void merge_horizontal(start, end, d)
{
  for (int i = 0; i < GRID_SIZE; ++i)
  {
    for (int j = start; (start > end) ? j > end : j < end; j += d)
    {
      if (game.grid[i][j] > 0 && game.grid[i][j] == game.grid[i][j + d])
      {
        game.grid[i][j] *= 2;
        game.grid[i][j + d] = 0;
        game.score += game.grid[i][j];
      }
    }
  }
}

void merge_vertical(start, end, d)
{
  for (int i = start; (start > end) ? i > end : i < end; i += d)
  {
    for (int j = 0; j < GRID_SIZE; ++j)
    {
      if (game.grid[i][j] == game.grid[i + d][j])
      {
        game.grid[i][j] *= 2;
        game.grid[i + d][j] = 0;
        game.score += game.grid[i][j];
      }
    }
  }
}

void merge(direction)
{
  switch (direction)
  {
  case D_RIGHT:
    merge_horizontal(GRID_SIZE - 1, 0, -1);
    break;
  case D_LEFT:
    merge_horizontal(0, GRID_SIZE - 1, 1);
    break;
  case D_DOWN:
    merge_vertical(GRID_SIZE - 1, 0, -1);
    break;
  case D_UP:
    merge_vertical(0, GRID_SIZE - 1, 1);
    break;

  default:
    break;
  }
}

void update(int direction)
{
  if (game.state == GAME_WON || game.state == GAME_LOST) {
    return;
  }
  move(direction);
  merge(direction);
  move(direction);
  spawn_block();
  draw();
}

int get_key()
{
  int value = getchar();

  if (value == ARROW_KEY && getchar() == ARROW_KEY_OPT)
  {
    value = getchar();
  }

  return value;
}

void loop()
{
  while (1)
  {
    int key = get_key();
    int direction = -1;

    switch (key)
    {
    case EXIT_KEY:
      return;

    case RESTART_KEY:
      init();
      break;

    case LEFT_KEY:
      direction = D_LEFT;
      break;

    case RIGHT_KEY:
      direction = D_RIGHT;
      break;

    case DOWN_KEY:
      direction = D_DOWN;
      break;

    case UP_KEY:
      direction = D_UP;
      break;

    default:
      break;
    }

    if (direction > 0)
    {
      update(direction);
      game.state = get_game_state();

      if (game.state == GAME_WON)
      {
        printf("Congrats, nerd!\n");
      }
      else if (game.state == GAME_LOST)
      {
        printf("You suck!\n");
      }
    }
  }
}

void init()
{
  for (int i = 0; i < GRID_SIZE; ++i)
  {
    for (int j = 0; j < GRID_SIZE; ++j)
    {
      game.grid[i][j] = 0;
    }
  }

  game.state = GAME_PROCESS;

  spawn_block();
  spawn_block();

  // int grid[GRID_SIZE][GRID_SIZE] = {
  //   {0,0,0,0,0},
  //   {0,2,2,2,0},
  //   {0,0,2,0,0},
  //   {0,0,2,0,0},
  //   {0,0,0,0,0},
  // };

  // for (int i = 0; i < GRID_SIZE; ++i)
  // {
  //   for (int j = 0; j < GRID_SIZE; ++j)
  //   {
  //     game.grid[i][j] = grid[i][j];
  //   }
  // }

  draw();
}

int main()
{
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  srand(time(0));

  init();
  loop();

  clear_console();
  printf("\n\n🤪🤪🤪\n\n");

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return 0;
}

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

typedef struct {
  int x, y;
} Vec2;

typedef struct Snake {
  int x, y;
  struct Snake *next;
} Snake;

int rows, cols, score, ended, quit_update;
Vec2 vel, last_vel, apple;
Snake *snake;
pthread_t update_thread_id;

void
snake_add (int x, int y)
{
  Snake *new_node = (Snake *) malloc (sizeof (Snake));
  new_node->x = x;
  new_node->y = y;
  new_node->next = NULL;

  if (snake == NULL) snake = new_node;
  else
    {
      Snake *temp = snake;
      while (temp->next != NULL) temp = temp->next;
      temp->next = new_node;
    }
}

void
snake_free (Snake *head)
{
  Snake *current = head;
  Snake *next_node;

  while (current != NULL)
    {
      next_node = current->next;
      free (current);
      current = next_node;
    }
}

void
snake_update_position ()
{
  Snake *temp = snake;
  int prev_x, prev_y, next_x, next_y;
  prev_x = temp->x;
  prev_y = temp->y;

  temp->x += vel.x;
  temp->y += vel.y;

  temp = temp->next;

  while (temp != NULL)
    {
      next_x = temp->x;
      next_y = temp->y;

      temp->x = prev_x;
      temp->y = prev_y;

      prev_x = next_x;
      prev_y = next_y;

      temp = temp->next;
    }
  last_vel = vel;
}

int
is_inside_snake (int x, int y)
{
  Snake *temp = snake;
  while (temp != NULL)
    {
      if (temp->x == x && temp->y == y) return 1;
      temp = temp->next;
    }
  return 0;
}

void
gen_apple ()
{
  int max_attempts = rows * cols;
  int attempts = 0;
  do {
    apple.x = rand () % cols;
    if (apple.x % 2 != 0) apple.x--;
    apple.y = rand () % rows;
    if (apple.y % 2 != 0) apple.y--;
    attempts ++;
  } while (is_inside_snake (apple.x, apple.y) && attempts < max_attempts);

  if (attempts >= max_attempts) pthread_exit (NULL);
}

void
draw_score ()
{
  char s[50];
  sprintf (s, "score: %d", score);
  mvprintw (rows - 1, cols - strlen (s), s);
}

void *
update ()
{
  struct timespec sleep_time;
  sleep_time.tv_nsec = 100000000;

  gen_apple ();

  for (;;)
    {
    loop_start:
      nanosleep (&sleep_time, NULL);

      if (quit_update)
        {
          snake_free (snake);
          snake = NULL;
          return NULL;
        }

      if (ended) continue;
      if (snake == NULL) snake_add(10, 5);
      clear ();
      draw_score ();

      mvaddch (apple.y, apple.x, ACS_DIAMOND);

      snake_update_position ();

      if (snake->x < 0) snake->x = cols - 2 + (cols % 2);
      else if (snake->x >= cols) snake->x = 0;
      if (snake->y < 0) snake->y = rows - 1;
      else if (snake->y >= rows) snake->y = 0;

      /* Check if the snake is eating itself */
      Snake *temp = snake->next;
      while (temp != NULL)
        {
          if (temp->x == snake->x && temp->y == snake->y)
            {
              snake_free (snake);
              snake = NULL;
              ended = 1;
              mvprintw (rows - 1, 0, "(q)uit or (r)estart");
              goto loop_start;
            }
          temp = temp->next;
        }

      /* Eat apple */
      if (snake->y == apple.y && snake->x == apple.x)
        {
          score ++;
          gen_apple ();
          snake_add (snake->x, snake->y);
        }

      temp = snake;
      while (temp != NULL)
        {
          mvaddch (temp->y, temp->x, '#');
          temp = temp->next;
        }

      refresh ();
    }
  return NULL;
}

void
loop ()
{
  pthread_create (&update_thread_id, NULL, update, NULL);

  for (char c;;)
    {
      c = getch();
      if (c == 'q') return;

      if (ended && c == 'r') ended = score = 0;

      vel.y = vel.x = 0;

      if (c == 'i' && last_vel.y !=  1) vel.y = -1;
      if (c == 'k' && last_vel.y != -1) vel.y =  1;
      if (c == 'j' && last_vel.x !=  2) vel.x = -2;
      if (c == 'l' && last_vel.x != -2) vel.x =  2;

      if (!vel.y && !vel.x) vel = last_vel;
    }
}

void
start ()
{
  initscr ();
  getmaxyx (stdscr, rows, cols);
  snake = NULL;
  noecho ();
  curs_set (0);
  srand (time (NULL));
}

void
end ()
{
  quit_update = 1;
  pthread_join (update_thread_id, NULL);
  endwin ();
}

int
main ()
{
  start ();
  loop ();
  end ();
  return 0;
}

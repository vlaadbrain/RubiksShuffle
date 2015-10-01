/*
 * main.c
 * Shuffle Your Rubik's Cube!
 * based entirely on work from http://shuffle.akselipalen.com/
 * 
 * This app runs on a pebble.  it will loop ever through the random moves
 * generated.  
 * - the up button decreases the interval by a second down to 1 second between moves
 * - the down button increases the interval.
 * - the select button toggle to pause/unpause on the current move.
 * - the back button exits
 *
 * the pseudo-random number generator is seeded at initialization.
 */

#include <pebble.h>
  
static const int16_t MOVES_AVAILABLE = 11;
static const int16_t SUCCESSORS_AVAILABLE = 8;
static const int16_t SUB_BITMAP_W = 100;
static const int16_t SUB_BITMAP_H = 100;

static Window *s_main_window;
static TextLayer *s_move_name_layer;
static GFont s_move_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_rotations_bitmap;
static GBitmap *s_rotation;

static int16_t interval = 5;
static int16_t interval_counter = 0;
static int16_t current_move;
static bool pause = false;

typedef struct {
  const char key;
  const char *name;
  int16_t x;
  int16_t y;
  const char successors[8];
} Move;

static const Move moves[] = {
  { 'f', "F" ,  0,   0, {'l', 'L', 'r', 'R', 'u', 'U', 'd', 'D'}},
  { 'F', "F'",  0, 100, {'l', 'L', 'r', 'R', 'u', 'U', 'd', 'D'}},
  { 'b', "B" ,100,   0, {'l', 'L', 'r', 'R', 'u', 'U', 'd', 'D'}}, 
  { 'B', "B'",100, 100, {'l', 'L', 'r', 'R', 'u', 'U', 'd', 'D'}},
  { 'l', "L" ,200, 100, {'f', 'F', 'b', 'B', 'u', 'U', 'd', 'D'}},
  { 'L', "L'",200,   0, {'f', 'F', 'b', 'B', 'u', 'U', 'd', 'D'}},
  { 'r', "R" ,300,   0, {'f', 'F', 'b', 'B', 'u', 'U', 'd', 'D'}},
  { 'R', "R'",300, 100, {'f', 'F', 'b', 'B', 'u', 'U', 'd', 'D'}},
  { 'u', "U" ,400,   0, {'f', 'F', 'b', 'B', 'l', 'L', 'r', 'R'}},
  { 'U', "U'",400, 100, {'f', 'F', 'b', 'B', 'l', 'L', 'r', 'R'}},
  { 'd', "D" ,500,   0, {'f', 'F', 'b', 'B', 'l', 'L', 'r', 'R'}},
  { 'D', "D'",500, 100, {'f', 'F', 'b', 'B', 'l', 'L', 'r', 'R'}},
};

static int16_t random_n(int16_t n) {
  return (rand() % n);
}

static int index_of_key(char key) {
  for (int16_t i = 0; i < MOVES_AVAILABLE; i++) {
    if (moves[i].key == key) 
      return i;
  }
  return 0;
}

static void update_move() {
  int16_t random_move = random_n(SUCCESSORS_AVAILABLE);
  const Move *cm, *sm;
  cm = &moves[current_move];
  current_move = index_of_key(cm->successors[random_move]);
  sm = &moves[current_move];
    
  if (s_rotation != NULL) {
    gbitmap_destroy(s_rotation);  
  }

  s_rotation = gbitmap_create_as_sub_bitmap(s_rotations_bitmap, GRect(sm->x, sm->y, SUB_BITMAP_W, SUB_BITMAP_H));
  bitmap_layer_set_bitmap(s_background_layer, s_rotation);
  text_layer_set_text(s_move_name_layer, sm->name);
  interval_counter = interval;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (interval == 0) {
    return;
  } else {
    interval--;
  }
  update_move();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (pause) {
    pause = false;
  } else {
    pause = true;
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  interval++;
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  s_rotations_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROTATIONS);
  s_background_layer = bitmap_layer_create(GRect(44,68,100,100));
  bitmap_layer_set_alignment(s_background_layer, GAlignBottomRight);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  s_move_name_layer = text_layer_create(GRect(0, 30, 72, 50));
  s_move_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  text_layer_set_background_color(s_move_name_layer, GColorBlack);
  text_layer_set_text_color(s_move_name_layer, GColorWhite);
  
  text_layer_set_font(s_move_name_layer, s_move_font);
  text_layer_set_text_alignment(s_move_name_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_move_name_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_move_name_layer);
  fonts_unload_custom_font(s_move_font);
  
  if (s_rotation != NULL) {
   gbitmap_destroy(s_rotation); 
  }
  gbitmap_destroy(s_rotations_bitmap);
  
  bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (pause) return;
  if (interval_counter <= 0) {
    update_move();
  } else {
    interval_counter--;
  }
}

static void init() {
  srand(time(NULL));
  current_move = random_n(MOVES_AVAILABLE);
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(s_main_window, click_config_provider); 
  window_set_background_color(s_main_window, GColorBlack);
  window_stack_push(s_main_window, true);
  update_move();
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
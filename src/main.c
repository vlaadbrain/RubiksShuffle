/*
 * main.c
 * Shuffle Your Rubik's Cube!
 * based entirely on work from http://shuffle.akselipalen.com/
 * 
 * This app runs on a pebble.  it will loop ever through the random moves
 * generated.  
 * - the select button shows menu to choose speed
 * - the back button exits
 *
 * the pseudo-random number generator is seeded at initialization.
 *
 *****
 * TODOS: 
 *   - cleanup a little
 */

#include <pebble.h>
  
#define LENGTH(X) (sizeof X/ sizeof X[0])

#define SPEED_MENU_SECTIONS 1
#define SPEED_MENU_ITEMS 4
  
static const int16_t RUBIKS_MOVES_AVAILABLE = 12;
static const int16_t RUBIKS_SUCCESSORS_AVAILABLE = 8;
static const uint32_t RUBIKS_SHUFFLE_KEY = 0xffddfdfd;
static const int32_t RUBIKS_DEFAULT_SPEED = 3;
static const int16_t SUB_BITMAP_W = 100;
static const int16_t SUB_BITMAP_H = 100;

static Window *s_main_window;
static TextLayer *s_move_name_layer;
static GFont s_move_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_rotations_bitmap;
static GBitmap *s_rotation;

static Window *s_speed_menu_window;
static MenuLayer *s_speed_menu_layer;

static int16_t interval = 1;
static int16_t interval_counter = 0;
static int16_t current_move;

typedef struct {
  const char *level;
  int16_t interval;
} Speed;

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

static const Speed speeds[] = {
  { "Sprint" , 0 },
  { "Run"    , 1 },
  { "Walk"   , 3 },
  { "Mosey"  , 5 },
};

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return SPEED_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return SPEED_MENU_ITEMS;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Select speed");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
  menu_cell_title_draw(ctx, cell_layer, speeds[cell_index->row].level);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  persist_write_int(RUBIKS_SHUFFLE_KEY, speeds[cell_index->row].interval);
  window_stack_pop(true);
}

static void set_selected_speed() {
  MenuIndex menu_index;
  menu_index.section = 0;
  menu_index.row = 0;
  
  for(uint16_t i=0;i<LENGTH(speeds); i++) {
    if (speeds[i].interval == interval) {
      menu_index.row = i;
      break;
    }
  }
  menu_layer_set_selected_index(s_speed_menu_layer, menu_index, MenuRowAlignCenter, true);
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_speed_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_speed_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  menu_layer_set_click_config_onto_window(s_speed_menu_layer, window);
  set_selected_speed();
  layer_add_child(window_layer, menu_layer_get_layer(s_speed_menu_layer));
}

static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_speed_menu_layer);
  
  window_destroy(window);
  s_speed_menu_window = NULL;
}

static void speed_menu_window_push() {
  if (!s_speed_menu_window) {
    s_speed_menu_window = window_create();
    window_set_window_handlers(s_speed_menu_window, (WindowHandlers) {
      .load = menu_window_load,
      .unload = menu_window_unload,
    });
  }
  
  window_stack_push(s_speed_menu_window, true);
}

static int16_t random_n(int16_t n) {
  return (rand() % n);
}

static int index_of_key(char key) {
  for (uint i = 0; i < LENGTH(moves); i++) {
    if (moves[i].key == key) 
      return i;
  }
  return 0;
}

static void update_move() {
  int16_t random_move = random_n(RUBIKS_SUCCESSORS_AVAILABLE);
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (interval_counter <= 0) {
    update_move();
  } else {
    interval_counter--;
  }
}

static void main_window_load(Window *window) {
  s_rotations_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROTATIONS);
  s_background_layer = bitmap_layer_create(GRect(44,53,100,100));
  bitmap_layer_set_alignment(s_background_layer, GAlignBottomRight);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  s_move_name_layer = text_layer_create(GRect(0, 15, 72, 50));
  s_move_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  text_layer_set_background_color(s_move_name_layer, GColorBlack);
  text_layer_set_text_color(s_move_name_layer, GColorWhite);
  
  text_layer_set_font(s_move_name_layer, s_move_font);
  text_layer_set_text_alignment(s_move_name_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_move_name_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_move_name_layer);
  
  if (s_rotation != NULL) {
   gbitmap_destroy(s_rotation); 
  }
  gbitmap_destroy(s_rotations_bitmap);
  
  bitmap_layer_destroy(s_background_layer);
}

static void main_window_appear(Window *window) {
  /* init random moves */
  srand(time(NULL));
  current_move = random_n(RUBIKS_MOVES_AVAILABLE);
  
  if (persist_exists(RUBIKS_SHUFFLE_KEY)) {
    interval = persist_read_int(RUBIKS_SHUFFLE_KEY);
  } else {
    interval = RUBIKS_DEFAULT_SPEED;
  }
  interval_counter = 0;
  
  /* generate initial move */
  update_move();
  
  /* subscribe callback for update_move */
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void main_window_disappear(Window *window) {
  tick_timer_service_unsubscribe();
}

static void main_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  speed_menu_window_push();
}

static void main_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, main_window_select_click_handler);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .appear = main_window_appear,
    .disappear = main_window_disappear,
    .unload = main_window_unload
  });
  window_set_background_color(s_main_window, GColorBlack);
  window_set_click_config_provider(s_main_window, main_window_click_config_provider);
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
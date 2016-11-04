#include <pebble.h>
#include "config.h"

#define STR_SIZE 20
#define TIME_OFFSET_PERSIST 1
#define REDRAW_INTERVAL 15

#if PBL_PLATFORM_CHALK
  #define WIDTH 144
#elif defined PBL_PLATFORM_EMERY
  #define WIDTH 144
#else
  #define WIDTH 144
#endif

#if PBL_PLATFORM_CHALK
  #define HEIGHT 72
#elif defined PBL_PLATFORM_EMERY
  #define HEIGHT 72
#else
  #define HEIGHT 72
#endif

#if PBL_PLATFORM_CHALK
  #define MAP_Y_START 100
#elif defined PBL_PLATFORM_EMERY
  #define MAP_Y_START 96
#else
  #define MAP_Y_START 96
#endif


static Window *window;
static TextLayer *time_text_layer;
static TextLayer *date_text_layer;
#ifdef PBL_BW
static GBitmap *world_bitmap;
#else
static GBitmap *three_worlds;
#endif
static Layer *canvas;
static GBitmap *image;
static int redraw_counter;
// s is set to memory of size STR_SIZE, and temporarily stores strings
char *s;
#ifdef PBL_SDK_2
// Local time is wall time, not UTC, so an offset is used to get UTC
int time_offset;
#endif

static TextLayer *time_text_layer;
static TextLayer *time_NY_text_layer;
static TextLayer *time_JPN_text_layer;
static TextLayer *time_GMT_text_layer;
static TextLayer *date_text_layer;
static TextLayer *s_battery_layer;

static GBitmap *background_image;
static BitmapLayer *background_layer;
static GFont custom_font;
static GFont custom_font2;

GBitmap *img_battery_100;
GBitmap *img_battery_90;
GBitmap *img_battery_80;
GBitmap *img_battery_70;
GBitmap *img_battery_60;
GBitmap *img_battery_50;
GBitmap *img_battery_40;
GBitmap *img_battery_30;
GBitmap *img_battery_20;
GBitmap *img_battery_10;
GBitmap *img_battery_charge;
BitmapLayer *layer_batt_img;



static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "10+";

  if (charge_state.is_charging) {
    //snprintf(battery_text, sizeof(battery_text), "++");
    snprintf(battery_text, sizeof(battery_text), "%d+", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
	    
		
        if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_20);
		} else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_50);
        } else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_60);
		} else	if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_90);
		} else if (charge_state.charge_percent <= 99) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
		} else {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
        } 
  }
  memmove(battery_text, &battery_text[0], sizeof(battery_text) - 1);
  text_layer_set_text(s_battery_layer, battery_text);
}

static void draw_earth() {
  // ##### calculate the time
#ifdef PBL_SDK_2
  int now = (int)time(NULL) + time_offset;
#else
  int now = (int)time(NULL);
#endif
  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)now / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = now - (((int)((float)now / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  // ##### draw the bitmap
  int x, y;
  for(x = 0; x < WIDTH; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(WIDTH));
    for(y = 0; y < HEIGHT; y++) {
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(HEIGHT * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
#ifdef PBL_BW
      int byte = y * gbitmap_get_bytes_per_row(image) + (int)(x / 8);
      if ((angle < 0) ^ (0x1 & (((char *)gbitmap_get_data(world_bitmap))[byte] >> (x % 8)))) {
        // white pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] & ~(0x1 << (x % 8));
      }
#else
      int byte = y * gbitmap_get_bytes_per_row(three_worlds) + x;
      if (angle < 0) { // dark pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT + byte];
      } else { // light pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT*2 + byte];
      }
#endif
    }
  }
  layer_mark_dirty(canvas);
}

static void draw_watch(struct Layer *layer, GContext *ctx) {
 // graphics_draw_bitmap_in_rect(ctx, image, gbitmap_get_bounds(image));
	
  Layer *window_bounds = window_get_root_layer(window);
  GRect image_bounds = layer_get_bounds(window_bounds);

  graphics_draw_bitmap_in_rect(ctx, image, GRect(0, MAP_Y_START, image_bounds.size.w, image_bounds.size.h));

}


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  handle_battery(battery_state_service_peek());
  //static char time_text[8] = "00:00 am";
  static char time_text[] = "00:00xx";
  static char time_JPN_text[] = "00:00xx"; //BEN ADDED
  static char time_NY_text[] = "00:00xx"; //BEN ADDED
  static char time_GMT_text[] = "00:00xx"; //BEN ADDED
  static char date_text[] = "Xxx, Xxx 00";
  static struct tm *JPN_time; //BEN ADDED
  static struct tm *NY_time; //BEN ADDED
  static struct tm *GMT_time; //BEN ADDED
 
  char *time_format;
	
	
  if (clock_is_24h_style()) {
        time_format = "%R";		
    } else {
        time_format = "%I:%M";
	
	}
    strftime(time_text, sizeof(time_text), time_format, tick_time);

	
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    text_layer_set_text(time_text_layer, time_text);	
	
  strftime(date_text, sizeof(date_text), "%a, %b %e", tick_time);
  text_layer_set_text(date_text_layer, date_text);

  //new york START
  NY_time = tick_time;
  NY_time->tm_hour += 9;  // might need to adjust values
  if (NY_time->tm_hour > 23) {
   NY_time->tm_hour -= 24;
  }  
  strftime(time_NY_text, sizeof(time_NY_text), "%R", NY_time);
  text_layer_set_text(time_NY_text_layer, time_NY_text);
  //new york END//
  
  //JAPAN START
  JPN_time = tick_time;
  JPN_time->tm_hour += 13;  // might need to adjust values
  if (JPN_time->tm_hour > 23) {
   JPN_time->tm_hour -= 24;
  }  
  strftime(time_JPN_text, sizeof(time_JPN_text), "%R", JPN_time);
  text_layer_set_text(time_JPN_text_layer, time_JPN_text);
  //JAPAN END//  
 
  //GMT START
  GMT_time = tick_time;
  GMT_time->tm_hour += 15;  // might need to adjust values
  if (GMT_time->tm_hour > 23) {
   GMT_time->tm_hour -= 24;
  }  
  strftime(time_GMT_text, sizeof(time_GMT_text), "%R", GMT_time);
  text_layer_set_text(time_GMT_text_layer, time_GMT_text);
  //GMT END//  
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth();
    redraw_counter = 0;
  } 

}

#ifdef PBL_SDK_2
// Get the time from the phone, which is probably UTC
// Calculate and store the offset when compared to the local clock
static void app_message_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_find(iterator, 0);
  int unixtime = t->value->int32;
  int now = (int)time(NULL);
  time_offset = unixtime - now;
  status_t s = persist_write_int(TIME_OFFSET_PERSIST, time_offset); 
  if (s) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved time offset %d with status %d", time_offset, (int) s);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to save time offset with status %d", (int) s);
  }
  draw_earth();
}
#endif


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_14));
  custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_32));
	
  window_set_background_color(window, GColorWhite );
	
  GRect bounds = layer_get_bounds(window_layer);

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2);
  GRect bitmap_bounds = gbitmap_get_bounds(background_image);
  GRect frame = GRect(0, 0, bitmap_bounds.size.w, bitmap_bounds.size.h);
  background_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
  
 //Local
  time_text_layer = text_layer_create(GRect(0, 33, 144-0, 168-0));//20150614 (GRect(0, 72, 144-0, 168-72));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_text_layer, GColorWhite);
  text_layer_set_text_color(time_text_layer, GColorBlack);
#else
  text_layer_set_background_color(time_text_layer, GColorClear );
  text_layer_set_text_color(time_text_layer, GColorBlack);
#endif
  #ifdef PBL_PLATFORM_APLITE
    text_layer_set_font(time_text_layer, custom_font2);//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #else
    text_layer_set_font(time_text_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS  ));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #endif  
  text_layer_set_text(time_text_layer, "");
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  //Date
  date_text_layer = text_layer_create(GRect(0,24, 144-0, 168-120));//20150614 (GRect(0, 110, 144-0, 168-72));

#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(date_text_layer, GColorWhite);
  text_layer_set_text_color(date_text_layer, GColorBlack);
#else
  text_layer_set_background_color(date_text_layer, GColorClear );
  text_layer_set_text_color(date_text_layer, GColorBlack);
#endif
  text_layer_set_font(date_text_layer, custom_font);
//  text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21 ));
  text_layer_set_text(date_text_layer, "");
  text_layer_set_text_alignment(date_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));

	
	//new york
  time_NY_text_layer = text_layer_create(GRect(2, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_NY_text_layer, GColorClear);
  text_layer_set_text_color(time_NY_text_layer, GColorBlack);
#else
  text_layer_set_background_color(time_NY_text_layer, GColorClear );
  text_layer_set_text_color(time_NY_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_NY_text_layer, custom_font);
  text_layer_set_text(time_NY_text_layer, "");
  text_layer_set_text_alignment(time_NY_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(time_NY_text_layer));  
  
  //JAPAN
  time_JPN_text_layer = text_layer_create(GRect(92, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_JPN_text_layer, GColorClear);
  text_layer_set_text_color(time_JPN_text_layer, GColorBlack);
#else
   text_layer_set_background_color(time_JPN_text_layer, GColorClear );
  text_layer_set_text_color(time_JPN_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_JPN_text_layer, custom_font);
  text_layer_set_text(time_JPN_text_layer, "");
  text_layer_set_text_alignment(time_JPN_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(time_JPN_text_layer));

 //GMT
  time_GMT_text_layer = text_layer_create(GRect(48, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_GMT_text_layer, GColorClear);
  text_layer_set_text_color(time_GMT_text_layer, GColorBlack);
#else
  text_layer_set_background_color(time_GMT_text_layer, GColorClear );
  text_layer_set_text_color(time_GMT_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_GMT_text_layer, custom_font);
  text_layer_set_text(time_GMT_text_layer, "");
  text_layer_set_text_alignment(time_GMT_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_GMT_text_layer));
	
   //BATTERY TEXT
  s_battery_layer = text_layer_create(GRect(40, 6, 30, 20));
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_background_color(s_battery_layer, GColorClear);//GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text(s_battery_layer, "  -");
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_100);
  img_battery_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_90);
  img_battery_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_80);
  img_battery_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_70);
  img_battery_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_60);
  img_battery_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_50);
  img_battery_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_40);
  img_battery_30   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_30);
  img_battery_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_20);
  img_battery_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_10);
  img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);

  layer_batt_img  = bitmap_layer_create(GRect(72, 11, 19, 9));
  bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
  layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
	
  canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(canvas, draw_watch);
  layer_add_child(window_layer, canvas);
#ifdef PBL_BW
  image = gbitmap_create_blank(GSize(WIDTH, HEIGHT), GBitmapFormat1Bit);
#else
  image = gbitmap_create_as_sub_bitmap(three_worlds, GRect(0, 0, WIDTH, HEIGHT));
#endif
  draw_earth();
  
}

static void window_unload(Window *window) {
	
  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);
	
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_90);
  gbitmap_destroy(img_battery_80);
  gbitmap_destroy(img_battery_70);
  gbitmap_destroy(img_battery_60);
  gbitmap_destroy(img_battery_50);
  gbitmap_destroy(img_battery_40);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_20);
  gbitmap_destroy(img_battery_10);
  gbitmap_destroy(img_battery_charge);
	
  text_layer_destroy(time_text_layer);
  text_layer_destroy(time_NY_text_layer);
  text_layer_destroy(time_JPN_text_layer);
  text_layer_destroy(time_GMT_text_layer);
  text_layer_destroy(date_text_layer);
  text_layer_destroy(s_battery_layer);
  fonts_unload_custom_font(custom_font);
  fonts_unload_custom_font(custom_font2);
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  layer_destroy(canvas);
 
}

static void init(void) {
  redraw_counter = 0;

#ifdef PBL_SDK_2
  // Load the UTC offset, if it exists
  time_offset = 0;
  if (persist_exists(TIME_OFFSET_PERSIST)) {
    time_offset = persist_read_int(TIME_OFFSET_PERSIST);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "loaded offset %d", time_offset);
  }
#endif

#ifdef PBL_BW
  world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
#else
  three_worlds = gbitmap_create_with_resource(RESOURCE_ID_THREE_WORLDS);
  //three_worlds = gbitmap_create_with_resource(RESOURCE_ID_NIGHT_PBL);
#endif

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);

  s = malloc(STR_SIZE);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

#ifdef PBL_SDK_2
  app_message_register_inbox_received(app_message_inbox_received);
  app_message_open(30, 0);
#endif
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  free(s);
  window_destroy(window);
#ifdef PBL_BW
  gbitmap_destroy(world_bitmap);
#else
  gbitmap_destroy(three_worlds);
#endif
}


int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}


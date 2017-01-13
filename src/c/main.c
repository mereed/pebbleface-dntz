/*

Copyright (C) 2017 David Gianforte, Ben, Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-------------------------------------------------------------------

*/

#include <pebble.h>
#include "main.h"

#define STR_SIZE 20
#define TIME_OFFSET_PERSIST 1
#define REDRAW_INTERVAL 15

#define WIDTH 144
#define HEIGHT 72

#if PBL_PLATFORM_CHALK
  #define MAP_Y_START  107
#else
  #define MAP_Y_START 102
#endif

static Window *window;

#ifdef PBL_BW
static GBitmap *world_bitmap;
#else
static GBitmap *three_worlds;
#endif

static Layer *s_line_layer1, *s_line_layer2;

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
static TextLayer *time_CITY1_text_layer;
static TextLayer *CITY1txt;
static TextLayer *time_CITY2_text_layer;
static TextLayer *CITY2txt;
static TextLayer *time_CITY3_text_layer;
static TextLayer *CITY3txt;
static TextLayer *date_text_layer;
static TextLayer *s_battery_layer;
static TextLayer *text_ampm_layer;

static GFont custom_font;
static GFont custom_font2;
static GFont custom_font4;


// A struct for our specific settings (see main.h)
ClaySettings settings;

int stringToInt(char *str);



// Initialize the default settings
static void prv_default_settings() {	
  settings.mapstyle = 0;
  settings.bluetoothvibe = false;
  settings.hourlyvibe = false;
//  settings.timezone = 0;
//  settings.timezoneOffset = 0;
}

// Read settings from persistent storage
static void prv_load_settings() {
  // Load the default settings
  prv_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  // Update the display based on new settings
  prv_update_display();
}

// Update the display elements
static void prv_update_display() {

	   
}	

int stringToInt(char *str){
    int i=0,sum=0;
    while(str[i]!='\0'){
         if(str[i]< 48 || str[i] > 57){
            // if (DEBUG) APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to convert it into integer.");
          //   return 0;
         }
         else{
             sum = sum*10 + (str[i] - 48);
             i++;
         }
    }
    return sum;
}

// Handle the response from AppMessage
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
	
  // mapstyle
  Tuple *map_t = dict_find(iter, MESSAGE_KEY_mapstyle);
  if (map_t) {
     settings.mapstyle = stringToInt((char*) map_t->value->data);
  }
  // Bluetoothvibe
  Tuple *bt_t = dict_find(iter, MESSAGE_KEY_bluetoothvibe);
  if (bt_t) {
    settings.bluetoothvibe = bt_t->value->int32 == 1;
  }

  // hourlyvibe
  Tuple *hourlyvibe_t = dict_find(iter, MESSAGE_KEY_hourlyvibe);
  if (hourlyvibe_t) {
    settings.hourlyvibe = hourlyvibe_t->value->int32 == 1;
  }
/*
 // timezone
  Tuple *digtime_t = dict_find(iter, MESSAGE_KEY_timezone);
  if (digtime_t) {
     settings.timezone = stringToInt((char*) digtime_t->value->data);
  }

 // timzone offset
  Tuple *day_t = dict_find(iter, MESSAGE_KEY_day);
  if (day_t) {
    settings.day = day_t->value->int32 == 1;
  }
*/
  // Save the new settings to persistent storage
  prv_save_settings();
}

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100+";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "%d+", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void toggle_bluetooth_icon(bool connected) {
if(!connected && settings.bluetoothvibe) {
    //vibe!
    vibes_short_pulse();
  }
  layer_set_hidden((s_line_layer1), !connected);
  layer_set_hidden((s_line_layer2), !connected);
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
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
  Layer *window_bounds = window_get_root_layer(window);
  GRect image_bounds = layer_get_bounds(window_bounds);
  graphics_draw_bitmap_in_rect(ctx, image, GRect(0, MAP_Y_START, image_bounds.size.w, image_bounds.size.h));
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

// Draw the horizontal lines
static void prv_line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

  static char ampm_text[] = "  ";
  static char time_text[] = "00:00xx";
  static char time_CITY2_text[] = "00:00xx"; 
  static char time_CITY1_text[] = "00:00xx"; 
  static char time_CITY3_text[] = "00:00xx"; 
  static char date_text[] = "Xxx, Xxx 00";
  static struct tm *CITY1_time; 
  static struct tm *CITY2_time; 
  static struct tm *CITY3_time; 
 
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

	
  // Update AM/PM indicator (i.e. AM or PM or nothing when using 24-hour style)
	// When in 12hr mode the indicator is moved so it looks nicer
	if ( !clock_is_24h_style() && time_text[0] == '1') {
#if PBL_PLATFORM_CHALK
		layer_set_frame(text_layer_get_layer(text_ampm_layer), GRect(147,55,30,30));
#else
		layer_set_frame(text_layer_get_layer(text_ampm_layer), GRect(122,52,30,30));
#endif
	} else {
#if PBL_PLATFORM_CHALK
		layer_set_frame(text_layer_get_layer(text_ampm_layer), GRect(138,55,30,20));
#else
		layer_set_frame(text_layer_get_layer(text_ampm_layer), GRect(113,52,30,20));
#endif
	}
  strftime( ampm_text, sizeof( ampm_text ), clock_is_24h_style() ? "" : "%p", tick_time );
  text_layer_set_text( text_ampm_layer, ampm_text );
	
	
  //NEW YORK START
  time_t now_1 = time(NULL);
  CITY1_time = gmtime(&now_1);
  CITY1_time->tm_hour += CITY1_offset;
  if (CITY1_time->tm_hour > 23) {
   CITY1_time->tm_hour -= 24;
  }  
  if (CITY1_time->tm_hour < 0) {
   CITY1_time->tm_hour += 24;
  }  
  strftime(time_CITY1_text, sizeof(time_CITY1_text), "%R", CITY1_time);
  text_layer_set_text(time_CITY1_text_layer, time_CITY1_text);
  //NEW YORK END//
	
	
  //JAPAN START
  time_t now_2 = time(NULL);
  CITY2_time = gmtime(&now_2);
  CITY2_time->tm_hour += CITY2_offset;
  if (CITY2_time->tm_hour > 23) {
   CITY2_time->tm_hour -= 24;
  }  
  if (CITY2_time->tm_hour < 0) {
   CITY2_time->tm_hour += 24;
  }  
  strftime(time_CITY2_text, sizeof(time_CITY2_text), "%R", CITY2_time);
  text_layer_set_text(time_CITY2_text_layer, time_CITY2_text);
  //JAPAN END//  
	
	
  //GMT/UTC START
  time_t now_3 = time(NULL);
  CITY3_time = gmtime(&now_3); 
  CITY3_time->tm_hour += CITY3_offset;
  strftime(time_CITY3_text, sizeof(time_CITY3_text), "%R", CITY3_time);
  text_layer_set_text(time_CITY3_text_layer, time_CITY3_text);
  //GMT END 
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth();
    redraw_counter = 0;
  } 
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_14));
  custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_40));
  custom_font4 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_44));
	
  window_set_background_color(window, GColorWhite );
	
  GRect bounds = layer_get_bounds(window_layer);

	//Local time
	
#if PBL_PLATFORM_CHALK
  time_text_layer = text_layer_create(GRect(0, 25, bounds.size.w, 52));
#else
  time_text_layer = text_layer_create(GRect(0, 26, bounds.size.w, 50));
#endif	
  text_layer_set_background_color(time_text_layer, GColorClear );
  text_layer_set_text_color(time_text_layer, GColorBlack);
#if PBL_PLATFORM_CHALK
  text_layer_set_font(time_text_layer, custom_font4);
#else
  text_layer_set_font(time_text_layer, custom_font2);
#endif  
  text_layer_set_text(time_text_layer, "");
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

//Date	
#if PBL_PLATFORM_CHALK
  date_text_layer = text_layer_create(GRect(0,20, bounds.size.w, 40));
#else
  date_text_layer = text_layer_create(GRect(0,21, bounds.size.w, 40));
#endif
  text_layer_set_background_color(date_text_layer, GColorClear );
  text_layer_set_text_color(date_text_layer, GColorBlack);
  text_layer_set_font(date_text_layer, custom_font);
  text_layer_set_text(date_text_layer, "");
  text_layer_set_text_alignment(date_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));

  // Setup AM/PM layer	
#ifdef PBL_PLATFORM_CHALK
  text_ampm_layer = text_layer_create(GRect(  138,  55,  30,  20 ));
#else
	text_ampm_layer = text_layer_create(GRect(  113, 52,  30,  20 ));	
#endif
  text_layer_set_background_color(text_ampm_layer, GColorClear);
  text_layer_set_text_color(text_ampm_layer, GColorBlack);
  text_layer_set_font(text_ampm_layer, custom_font);
  text_layer_set_text_alignment(text_ampm_layer, GTextAlignmentLeft);
  layer_add_child( window_layer, text_layer_get_layer( text_ampm_layer ) );	
	
	
	
	//new york
#if PBL_PLATFORM_CHALK
  time_CITY1_text_layer = text_layer_create(GRect(6, 90, 50, 16));
#else
  time_CITY1_text_layer = text_layer_create(GRect(2, 85, 50, 16));
#endif
  text_layer_set_background_color(time_CITY1_text_layer, GColorClear );
  text_layer_set_text_color(time_CITY1_text_layer, GColorBlack);
  text_layer_set_font(time_CITY1_text_layer, custom_font);
  text_layer_set_text(time_CITY1_text_layer, "");
  text_layer_set_text_alignment(time_CITY1_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(time_CITY1_text_layer));  
	
#if PBL_PLATFORM_CHALK
  CITY1txt = text_layer_create(GRect(6, 77, 174, 16));
#else
  CITY1txt = text_layer_create(GRect(2, 73, 142, 16)); 
#endif
  text_layer_set_background_color(CITY1txt, GColorClear);
  text_layer_set_text_color(CITY1txt, GColorBlack);
  text_layer_set_font(CITY1txt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(CITY1txt, CITY1_name);
  text_layer_set_text_alignment(CITY1txt, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(CITY1txt));  
	
  //JAPAN
#if PBL_PLATFORM_CHALK
  time_CITY2_text_layer = text_layer_create(GRect(124, 90, 50, 16));
#else
  time_CITY2_text_layer = text_layer_create(GRect(92, 85, 50, 16));
#endif
  text_layer_set_background_color(time_CITY2_text_layer, GColorClear );
  text_layer_set_text_color(time_CITY2_text_layer, GColorBlack);
  text_layer_set_font(time_CITY2_text_layer, custom_font);
  text_layer_set_text(time_CITY2_text_layer, "");
  text_layer_set_text_alignment(time_CITY2_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(time_CITY2_text_layer));

#if PBL_PLATFORM_CHALK
  CITY2txt = text_layer_create(GRect(0, 77, 174, 16));
#else
  CITY2txt = text_layer_create(GRect(0, 73, 142, 16));
#endif
  text_layer_set_background_color(CITY2txt, GColorClear);
  text_layer_set_text_color(CITY2txt, GColorBlack);
  text_layer_set_font(CITY2txt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(CITY2txt, CITY2_name);
  text_layer_set_text_alignment(CITY2txt, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(CITY2txt));  	
	
//GMT
#if PBL_PLATFORM_CHALK
  time_CITY3_text_layer = text_layer_create(GRect(0, 90, 180, 16));
#else
  time_CITY3_text_layer = text_layer_create(GRect(0, 85, 144, 16));
#endif
  text_layer_set_background_color(time_CITY3_text_layer, GColorClear );
  text_layer_set_text_color(time_CITY3_text_layer, GColorBlack);
  text_layer_set_font(time_CITY3_text_layer, custom_font);
  text_layer_set_text(time_CITY3_text_layer, "");
  text_layer_set_text_alignment(time_CITY3_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_CITY3_text_layer));
	
#if PBL_PLATFORM_CHALK
  CITY3txt = text_layer_create(GRect(0, 77, 180, 16));
#else
  CITY3txt = text_layer_create(GRect(0, 73, 144, 16));
#endif
  text_layer_set_background_color(CITY3txt, GColorClear);
  text_layer_set_text_color(CITY3txt, GColorBlack);
  text_layer_set_font(CITY3txt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(CITY3txt, CITY3_name);
  text_layer_set_text_alignment(CITY3txt, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(CITY3txt));  	
	
	
// The horizontal lines
#if PBL_PLATFORM_CHALK
  GRect line_frame = GRect(0, 77, bounds.size.w, 1);
#else
  GRect line_frame = GRect(0, 74, bounds.size.w, 1);
#endif
	s_line_layer1 = layer_create(line_frame);
  layer_set_update_proc(s_line_layer1, prv_line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer1);
	
 GRect line_frame2 = GRect(0, 18, bounds.size.w, 1);
  s_line_layer2 = layer_create(line_frame2);
  layer_set_update_proc(s_line_layer2, prv_line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer2);
		
//BATTERY TEXT
#if PBL_PLATFORM_CHALK
	s_battery_layer = text_layer_create(GRect(0, 1, 180, 20));
#else
	s_battery_layer = text_layer_create(GRect(0, 1, 144, 20));
#endif
	
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(canvas, draw_watch);
  layer_add_child(window_layer, canvas);
#ifdef PBL_BW
  image = gbitmap_create_blank(GSize(WIDTH, HEIGHT), GBitmapFormat1Bit);
#else
  image = gbitmap_create_as_sub_bitmap(three_worlds, GRect(0, 0, WIDTH, HEIGHT));
#endif
  draw_earth();
  
  handle_battery(battery_state_service_peek());
  toggle_bluetooth_icon(bluetooth_connection_service_peek());
}

static void window_unload(Window *window) {
	
  text_layer_destroy(time_text_layer);
  text_layer_destroy(time_CITY1_text_layer);
  text_layer_destroy(time_CITY2_text_layer);
  text_layer_destroy(time_CITY3_text_layer);
  text_layer_destroy(CITY1txt);
  text_layer_destroy(CITY2txt);
  text_layer_destroy(CITY3txt);
  text_layer_destroy(date_text_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(text_ampm_layer);

  layer_destroy(s_line_layer1);
  layer_destroy(s_line_layer2);

  layer_destroy(canvas);
 
}

static void init(void) {
  redraw_counter = 0;

  prv_load_settings();
	
  // Listen for AppMessages
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);

  // international support
  setlocale(LC_ALL, "");
	
	
#ifdef PBL_SDK_2
  // Load the UTC offset, if it exists
  time_offset = 0;
  if (persist_exists(TIME_OFFSET_PERSIST)) {
    time_offset = persist_read_int(TIME_OFFSET_PERSIST);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "loaded offset %d", time_offset);
  }
#endif

	switch (settings.mapstyle) {
			
		case 0: //sat-light 
		
		#ifdef PBL_BW
  			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NAT_DITHER_1);
		#else
  			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL);
		#endif
		break;
		
		case 1:  //sat-medium
		#ifdef PBL_BW
  			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NAT_DITHER_2);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL_V6);
		#endif
		break;
			
		case 2:  //sat-dark		
		#ifdef PBL_BW
  			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NAT_DITHER_3);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL_V5);
		#endif
		break;
		
		case 3:  //natural-grid 1 OR BW OUTLINE
		#ifdef PBL_BW
  			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_THREE_WORLDS);		
		#endif
		break;
		
		case 4:  //natural grid 2 OR BW GRID
		#ifdef PBL_BW
  			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BW_GRID);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL_V10);
		#endif
		break;
		
		case 5:  //simple colour, OR SOLID BLACK LAND
		#ifdef PBL_BW
			world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BW_SOLID);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_SIMPLE_DAY);
		#endif
		break;
			
		case 6:  //b&w, or OR SOLID WHITE LAND
 		#ifdef PBL_BW
		  	world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BW_INV);
		#else
			three_worlds = gbitmap_create_with_resource(RESOURCE_ID_BW);	
		#endif
		break;
	}
//#endif

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);

  s = malloc(STR_SIZE);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

//#ifdef PBL_SDK_2
//  app_message_register_inbox_received(app_message_inbox_received);
//  app_message_open(30, 0);
//#endif
	
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);	
	
  prv_update_display();
}

static void deinit(void) {

  app_message_deregister_callbacks();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  free(s);
#ifdef PBL_BW
  gbitmap_destroy(world_bitmap);
#else
  gbitmap_destroy(three_worlds);
#endif
	
  fonts_unload_custom_font(custom_font4);
  fonts_unload_custom_font(custom_font2);
  fonts_unload_custom_font(custom_font);
	
  window_destroy(window);
	
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

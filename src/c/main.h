#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1

// A structure containing our settings
typedef struct ClaySettings {
  int mapstyle;
  bool bluetoothvibe;
  bool hourlyvibe;
//  int timezone;
//  int timezoneOffset;

} __attribute__((__packed__)) ClaySettings;

// TODO make these configurable, not macros
// NOTE location on screen is CITY1 CITY3 CITY2
#define CITY1_name "NY (-)"
#define CITY1_offset -5  // NOTE not DST aware
#define CITY2_name "(+) Tokyo"
#define CITY2_offset 9
#define CITY3_name "UTC"
#define CITY3_offset 0

static void prv_default_settings();
static void prv_load_settings();
static void prv_save_settings();
static void prv_update_display();
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context);
static void prv_window_load(Window *window);
static void prv_window_unload(Window *window);
static void prv_init(void);
static void prv_deinit(void);

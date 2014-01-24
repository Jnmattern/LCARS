#include <pebble.h>

#define TOTAL_TIME_DIGITS 4
#define TOTAL_DIGITS 10
#define TOTAL_DATE_DIGITS 2


/* These are the variables we defined in our JavaScript earlier.  This will save
 us trying to work out where the cells should go. */
#define CELL_WIDTH 12
#define CELL_HEIGHT 2
#define CELL_PADDING_RIGHT 2
#define CELL_PADDING_TOP 2
#define CELL_OFFSET 33

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
    RESOURCE_ID_IMAGE_DAY_NAME_SUN,
    RESOURCE_ID_IMAGE_DAY_NAME_MON,
    RESOURCE_ID_IMAGE_DAY_NAME_TUE,
    RESOURCE_ID_IMAGE_DAY_NAME_WED,
    RESOURCE_ID_IMAGE_DAY_NAME_THU,
    RESOURCE_ID_IMAGE_DAY_NAME_FRI,
    RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
    RESOURCE_ID_IMAGE_DATENUM_0,
    RESOURCE_ID_IMAGE_DATENUM_1,
    RESOURCE_ID_IMAGE_DATENUM_2,
    RESOURCE_ID_IMAGE_DATENUM_3,
    RESOURCE_ID_IMAGE_DATENUM_4,
    RESOURCE_ID_IMAGE_DATENUM_5,
    RESOURCE_ID_IMAGE_DATENUM_6,
    RESOURCE_ID_IMAGE_DATENUM_7,
    RESOURCE_ID_IMAGE_DATENUM_8,
    RESOURCE_ID_IMAGE_DATENUM_9
};

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
    RESOURCE_ID_IMAGE_NUM_0,
    RESOURCE_ID_IMAGE_NUM_1,
    RESOURCE_ID_IMAGE_NUM_2,
    RESOURCE_ID_IMAGE_NUM_3,
    RESOURCE_ID_IMAGE_NUM_4,
    RESOURCE_ID_IMAGE_NUM_5,
    RESOURCE_ID_IMAGE_NUM_6,
    RESOURCE_ID_IMAGE_NUM_7,
    RESOURCE_ID_IMAGE_NUM_8,
    RESOURCE_ID_IMAGE_NUM_9
};

Window *window;
Layer *display_layer;

GBitmap *background_image;
GBitmap *day_name_image[7];
GBitmap *date_num_image[10];
GBitmap *big_digit_image[10];

BitmapLayer *background_layer;
BitmapLayer *day_name_layer;
BitmapLayer *date_digits_layer[TOTAL_DATE_DIGITS];
BitmapLayer *time_digits_layer[TOTAL_TIME_DIGITS];
BitmapLayer *digits[TOTAL_DIGITS];

static AppSync sync;
static uint8_t sync_buffer[64];
int TZOFFSET_KEY = 0;
int TZOffset = 0;

const int ROW[] = {
    12,
    63,
    114
};

const int COLUMN[] = {
    6,
    41,
    76,
    111
};

/* This is a port of the same function from the JS - it returns a GRect which
 is required to draw a rectangle on the display_layer. */
static GRect cell_location(int col, int row) {
    return GRect((col * (CELL_WIDTH + CELL_PADDING_RIGHT)) ,(CELL_OFFSET - (row * (CELL_HEIGHT + CELL_PADDING_TOP))), CELL_WIDTH, CELL_HEIGHT);
}

static void set_container_image(BitmapLayer *bmp_layer, GBitmap *bitmap, GPoint origin) {
	GRect frame;
	Layer *layer;
	
	bitmap_layer_set_bitmap(bmp_layer, bitmap);
    
	layer = bitmap_layer_get_layer(bmp_layer);
	frame = bitmap->bounds;
    frame.origin = origin;
    layer_set_frame(layer, frame);
}


static inline unsigned short get_display_hour(unsigned short hour) {
    if (clock_is_24h_style()) {
        return hour;
    }
    
    unsigned short display_hour = hour % 12;
    
    // Converts "0" to "12"
    return display_hour ? display_hour : 12;
}

int my_pow(int base, int exp) {
    int result = 1;
    for(int i=exp; i>0; i--) {
        result = result * base;
    }
    return result;
}

void update_display(time_t unix_time) {
	int i;
	struct tm *current_time = localtime(&unix_time);
    // TODO: Only update changed values?
    
    set_container_image(day_name_layer, day_name_image[current_time->tm_wday], GPoint(79, 3));
    
    // TODO: Remove leading zero?
    set_container_image(date_digits_layer[0], date_num_image[current_time->tm_mday/10], GPoint(111, 3));
    set_container_image(date_digits_layer[1], date_num_image[current_time->tm_mday%10], GPoint(118, 3));
    
    
    unsigned short display_hour = get_display_hour(current_time->tm_hour);
    
    // TODO: Remove leading zero?
    set_container_image(time_digits_layer[0], big_digit_image[display_hour/10], GPoint(38, 30));
    set_container_image(time_digits_layer[1], big_digit_image[display_hour%10], GPoint(62, 30));
    
    set_container_image(time_digits_layer[2], big_digit_image[current_time->tm_min/10], GPoint(86, 30));
    set_container_image(time_digits_layer[3], big_digit_image[current_time->tm_min%10], GPoint(110, 30));
    
    /* Convert time to seconds since epoch. */
    unix_time += TZOffset;
	
    /* Draw each digit in the correct location. */
    for(i=0; i<TOTAL_DIGITS; i++) {
        /* int digit_colum = i % TOTAL_COLUMNS; */
        int denominator = my_pow(10,i); /* The loop starts at the most significant digit and goes down from there. */
        int digit_value = (int)unix_time/(1000000000 / denominator); /* This gives the value for the current digit. (Casting should give us the floor of the value.) */
        unix_time = unix_time % (1000000000 / denominator); /* This subtracts the value for the current digit so that it doesn't interfere with the next iteration of the loop. */
        set_container_image(digits[i], date_num_image[digit_value], GPoint(40 + (i * 9), 90)); /* Now we set this digit. */
    }
    
    
}

void handle_tick(struct tm *t, TimeUnits units_changed) {
    update_display(time(NULL));

    /* Tell the application to rerender the layer */
    layer_mark_dirty(display_layer);
}

void display_layer_update_callback(Layer *me, GContext* ctx) {
	int cell_column_index;
	time_t unix_time = time(NULL);
	struct tm *t = localtime(&unix_time);
    
    unsigned short display_hour = get_display_hour(t->tm_hour);
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    
    for (cell_column_index = 0; cell_column_index < display_hour/10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(0, cell_column_index), 0, GCornerNone);
    }

    for (cell_column_index = 0; cell_column_index < display_hour%10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(1, cell_column_index), 0, GCornerNone);
    }
    
    for (cell_column_index = 0; cell_column_index < t->tm_min/10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(2, cell_column_index), 0, GCornerNone);
    }

    for (cell_column_index = 0; cell_column_index < t->tm_min%10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(3, cell_column_index), 0, GCornerNone);
    }
    
    for (cell_column_index = 0; cell_column_index < t->tm_sec/10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(4, cell_column_index), 0, GCornerNone);
    }
	
    for (cell_column_index = 0; cell_column_index < t->tm_sec%10; cell_column_index++) {
        graphics_fill_rect(ctx, cell_location(5, cell_column_index), 0, GCornerNone);
    }
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	TZOffset = new_tuple->value->int32;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received TZOffset = %d", TZOffset);
}

static void init() {
	int i;
	Layer *window_layer;
	GRect r = { {0, 0}, {1, 1} };
	
	Tuplet initial_values[] = { 	TupletInteger(TZOFFSET_KEY, 0) };
	
	app_message_open(64, 64);
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
				  sync_tuple_changed_callback, sync_error_callback, NULL);
	


	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	window_layer = window_get_root_layer(window);

	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
	bitmap_layer_set_bitmap(background_layer, background_image);
	
	display_layer = layer_create(GRect(43, 110, 82, 35));
	layer_set_update_proc(display_layer, display_layer_update_callback);

	layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
	layer_add_child(window_layer, display_layer);

	// Load all images
	for (i=0; i<7; i++) {
		day_name_image[i] = gbitmap_create_with_resource(DAY_NAME_IMAGE_RESOURCE_IDS[i]);
	}
	
	for (i=0; i<10; i++) {
		date_num_image[i] = gbitmap_create_with_resource(DATENUM_IMAGE_RESOURCE_IDS[i]);
		big_digit_image[i] = gbitmap_create_with_resource(BIG_DIGIT_IMAGE_RESOURCE_IDS[i]);
	}
	
	// Create BitmapLayers
	day_name_layer = bitmap_layer_create(r);
	layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));

	for (i=0; i<TOTAL_DATE_DIGITS; i++) {
		date_digits_layer[i] = bitmap_layer_create(r);
		layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layer[i]));
	}

	for (i=0; i<TOTAL_TIME_DIGITS; i++) {
		time_digits_layer[i] = bitmap_layer_create(r);
		layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layer[i]));
	}
	
	for (i=0; i<TOTAL_DIGITS; i++) {
		digits[i] = bitmap_layer_create(r);
		layer_add_child(window_layer, bitmap_layer_get_layer(digits[i]));
	}

	// Avoids a blank screen on watch start.
	update_display(time(NULL));
	
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}


static void deinit() {
	int i;

	app_sync_deinit(&sync);
	tick_timer_service_unsubscribe();
	
	for (i=0; i<TOTAL_DIGITS; i++) {
		bitmap_layer_destroy(digits[i]);
	}
	
	for (i=0; i<TOTAL_TIME_DIGITS; i++) {
		bitmap_layer_destroy(time_digits_layer[i]);
	}
	
	for (i=0; i<TOTAL_DATE_DIGITS; i++) {
		bitmap_layer_destroy(date_digits_layer[i]);
	}
	
	bitmap_layer_destroy(day_name_layer);
	
	for (i=0; i<7; i++) {
		gbitmap_destroy(day_name_image[i]);
	}
	
	for (i=0; i<10; i++) {
		gbitmap_destroy(date_num_image[i]);
		gbitmap_destroy(big_digit_image[i]);
	}
	
	layer_destroy(display_layer);
	bitmap_layer_destroy(background_layer);
	
	gbitmap_destroy(background_image);
	
	window_destroy(window);
}


int main(void) {
	init();
	app_event_loop();
	deinit();
}

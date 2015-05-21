#include "pebble.h"
#include "math.h"
#include "startline.h"

#ifdef PBL_COLOR
GColor8 normalTitleColour;
GColor8 invertedTitleColour;
GColor8 backgroundColour;
GColor8 normalTextColour;
GColor8 negativeTextColour;
GColor8 negativeBackgroundColour;
GColor8 greenTextColour;
GColor8 redTextColour;
GColor8 messageTextColour;
GColor8 messageOutlineColour;
GColor8 messageBackgroundColour;
#endif
  
  
GBitmap *s_res_padlock;
BitmapLayer *s_padlockLayer;



Window *s_main_window;
TextLayer *s_data_layer[16];
TextLayer *dataInverterPT[6];
TextLayer *s_data_title[20];
TextLayer *messageLayer;
TextLayer *messageOutline;
void doupdate();
void updatescreen();


int holdThisScreen = TRANSITION_IDLE; // When this is non-zero, no auto transition
int currentScreen = 0;
int configuring = 0; // Set to 1 when configuring display
int configuring_field = 0; // Index of the title we are currently configuring
bool doubleClick = false;
bool messageClick = false;


keyTitle keyTitles[] = {
{KEY_LAY_BURN,"Lay Burn", true, false},
{KEY_LAY_DIST,"Lay Dist", true, false},
{KEY_LAY_TIME,"Lay Time", true, false},
{KEY_LINE_BURN,"Line Burn", true, false},
{KEY_LINE_DIST, "Line Dist", true, false},
{KEY_LINE_TIME,"Line Time", true, false},
{KEY_LINE_ANGLE, "Line Angle", true, false},
{KEY_SECS_TO_START, "To Start", true, false},
{KEY_LAY_SEL, "Lay Line", true, false},

};
 
int num_keytitles = sizeof(keyTitles) / sizeof(keyTitles[0]);

#define THREE_FIELD_INDEX 12
  


Screen screens[NUM_SCREENS] = {
                                      {6,
                                      {0,1,2,3,4,5},
                                      {0,1,2,3,4,5},
                                      {0,1,2,3,4,5},
                                      true
                                      }
                                    };  // Code relies on the rest of the array being zero to indicate screens not in use.

static GFont s_2_font, s_title_font, s_4_font, s_3_font,
              s_6_font, s_medium_title_font, s_large_title_font;

TextLayer *flash;
static Layer *dataLayer, *titleLayer; /* Layers to hold all the titles & data - for Z control */

void doDataInvert(int field)  // Fixed up due to loss of inverter_layer
  {
  GRect a = layer_get_frame(text_layer_get_layer(s_data_layer[screens[currentScreen].field_layer_map[field]]));
  a.origin.y += a.size.h / 4;
  a.size.h -= a.size.h/4 ;
  int fd = keyTitles[screens[currentScreen].field_data_map[field]].key;
  if (fd == KEY_LAY_BURN || fd == KEY_LINE_BURN) {  // Set BURN fields green when negative
    text_layer_set_text_color(s_data_layer[screens[currentScreen].field_layer_map[field]], greenTextColour);
  }
  else { // All other fields invert (sort of!!)
//    layer_remove_from_parent(text_layer_get_layer(dataInverterPT[field])); 
    layer_set_frame(text_layer_get_layer(dataInverterPT[field]), a);
    layer_set_hidden(text_layer_get_layer(dataInverterPT[field]), false);
    text_layer_set_background_color(dataInverterPT[field], negativeBackgroundColour);
    // layer_insert_above_sibling(text_layer_get_layer(dataInverterPT[field]), text_layer_get_layer(s_data_layer[screens[currentScreen].field_layer_map[field]]));
    text_layer_set_text_color(s_data_layer[screens[currentScreen].field_layer_map[field]], negativeTextColour);
  }
  
}

void doDataRevert(int field)
  {
  layer_set_hidden(text_layer_get_layer(dataInverterPT[field]), true);
  text_layer_set_text_color(s_data_layer[screens[currentScreen].field_layer_map[field]], normalTextColour);
}

void draw_layer(Layer *layer, GContext *gctxt)
{
  if (!layer_get_hidden(layer)) {
      graphics_context_set_fill_color(gctxt, negativeBackgroundColour);
      GRect rect = layer_get_bounds(layer);
      graphics_fill_rect(gctxt, rect, 5, GCornersAll);
  }
}

static void main_window_load(Window *window) {
normalTitleColour = GColorWhite;
invertedTitleColour = (GColor8){.argb=((uint8_t)(0xC0|49))};
backgroundColour = GColorBlack;
normalTextColour = GColorWhite;
  flash = text_layer_create(GRect(2,7,7,7));
  text_layer_set_background_color(flash, GColorDarkCandyAppleRed);
  
  int jj;
  for (jj = 0; jj < 6; jj++) {
  dataInverterPT[jj] = text_layer_create(GRect(0,0,144,168));
  text_layer_set_background_color(dataInverterPT[jj], GColorBlack);
  // layer_set_bounds(text_layer_get_layer(dataInverterPT[jj]),GRectZero);  
  
  layer_set_update_proc(text_layer_get_layer(dataInverterPT[jj]), draw_layer);
  layer_set_hidden(text_layer_get_layer(dataInverterPT[jj]), true);
  }
  
  #define SIX_FIELD_INDEX 0
  #define SIX_FIELD_MAX 5
  
  // At least, for now, the six field layout does not scroll
  s_data_layer[0] = text_layer_create(GRect(0, 2, 71, 49));
  s_data_title[0] = text_layer_create(GRect(0, 49, 71, 15));

  s_data_layer[1] = text_layer_create(GRect(73, 2, 71, 49));
  s_data_title[1] = text_layer_create(GRect(73, 49, 71, 15));
  
  s_data_layer[2] = text_layer_create(GRect(0, 53, 71, 49));
  s_data_title[2] = text_layer_create(GRect(0, 102, 71, 14));
  
  s_data_layer[3] = text_layer_create(GRect(73, 53, 71, 49));
  s_data_title[3] = text_layer_create(GRect(73, 102, 71, 14));
  
  s_data_layer[4] = text_layer_create(GRect(0, 105, 71, 49));
  s_data_title[4] = text_layer_create(GRect(0, 154, 71, 15));
  
  s_data_layer[5] = text_layer_create(GRect(73, 105, 71, 49));
  s_data_title[5] = text_layer_create(GRect(73, 154, 71, 15));
  
  // Two data fields & their titles
  #define TWO_FIELD_INDEX 6
  #define TWO_FIELD_MAX 7
  s_data_layer[6] = text_layer_create(GRect(0, 2, 288, 60));
  layer_set_frame((Layer *) s_data_layer[6], GRect(0, 2, 144, 60));
  layer_set_bounds((Layer *) s_data_layer[6], GRect(0, 0, 288, 60));
  s_data_title[6] = text_layer_create(GRect(0, 64, 144, 28));
  
  s_data_layer[7] = text_layer_create(GRect(0, 79, 288, 60));
  layer_set_frame((Layer *) s_data_layer[7], GRect(0, 79, 144, 60));
  layer_set_bounds((Layer *) s_data_layer[7], GRect(0, 0, 288, 60));
  s_data_title[7] = text_layer_create(GRect(0, 140, 144, 28));

  
  // Four data fields & their titles
  #define FOUR_FIELD_INDEX 8
  #define FOUR_FIELD_MAX 11
  s_data_layer[8] = text_layer_create(GRect(0, 12, 142, 51));
  layer_set_frame((Layer *) s_data_layer[8], GRect(0, 12, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[8], GRect(0, 0, 142, 51));
  s_data_title[8] = text_layer_create(GRect(0, 65, 71, 24));
  
  s_data_layer[9] = text_layer_create(GRect(73, 12, 142, 51));
  layer_set_frame((Layer *) s_data_layer[9], GRect(73, 12, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[9], GRect(0, 0, 142, 51));
  s_data_title[9] = text_layer_create(GRect(73, 65, 71, 24));
  
  s_data_layer[10] = text_layer_create(GRect(0, 91, 142, 51));
  layer_set_frame((Layer *) s_data_layer[10], GRect(0, 91, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[10], GRect(0, 0, 142, 51));
  s_data_title[10] = text_layer_create(GRect(0, 144, 71, 24));
  
  
  s_data_layer[11] = text_layer_create(GRect(73, 91,  142, 51));
  layer_set_frame((Layer *) s_data_layer[11], GRect(73, 91, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[11], GRect(0, 0, 142, 51));
  
  s_data_title[11] = text_layer_create(GRect(73, 144, 71, 24));
  
  // Three fields - One big, two small
  //#define THREE_FIELD_INDEX 12
  #define THREE_FIELD_MAX 14
  s_data_layer[12] = text_layer_create(GRect(0, 10, 432, 65));
  layer_set_frame((Layer *) s_data_layer[12], GRect(0, 10, 144, 65));
  layer_set_bounds((Layer *) s_data_layer[12], GRect(0, 0, 432, 65));
  s_data_title[12] = text_layer_create(GRect(0, 75, 144, 28));

  s_data_layer[13] = text_layer_create(GRect(0, 91, 150, 51));
  layer_set_frame((Layer *) s_data_layer[13], GRect(0, 91, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[13], GRect(0, 0, 150, 51));
  s_data_title[13] = text_layer_create(GRect(0, 144, 71, 24));
  
  s_data_layer[14] = text_layer_create(GRect(73, 91, 142, 51));
  layer_set_frame((Layer *) s_data_layer[14], GRect(73, 91, 71, 51));
  layer_set_bounds((Layer *) s_data_layer[14], GRect(0, 0, 150, 51));
  s_data_title[14] = text_layer_create(GRect(73, 144, 71, 24));
  
  
  // Top title
  s_data_layer[TITLE_INDEX] = text_layer_create(GRect(0, 0, 144, 16));
  
  

  // Set up top title area

    text_layer_set_text_color(s_data_layer[TITLE_INDEX], normalTextColour);
    text_layer_set_text_alignment(s_data_layer[TITLE_INDEX], GTextAlignmentCenter);
    text_layer_set_text(s_data_layer[TITLE_INDEX], "StartLine");
    text_layer_set_font(s_data_layer[TITLE_INDEX], s_title_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_data_layer[TITLE_INDEX])); 
  text_layer_set_background_color(s_data_layer[TITLE_INDEX], GColorClear);
  window_set_background_color(window, backgroundColour);

  messageLayer = text_layer_create(GRect(10,30,124,120));
    messageOutline = text_layer_create(GRect(5,25,134,130));
    text_layer_set_background_color(messageOutline, GColorClear);
  text_layer_set_background_color(messageLayer, GColorClear);
  text_layer_set_text_color(messageLayer, messageTextColour);
  
  text_layer_set_text_alignment(messageLayer, GTextAlignmentCenter);
  text_layer_set_font(messageLayer, s_large_title_font);
  
  // Add the Title layer first so it is behind the data layer
  titleLayer = layer_create(GRect(0, 0, 144, 168));
  layer_insert_below_sibling(titleLayer, (Layer *)s_data_layer[TITLE_INDEX]);
  
  
  // Now do the data layer - in front of the title layer
  dataLayer = layer_create(GRect(0, 0, 144, 168)); 
  layer_insert_below_sibling(dataLayer, titleLayer); 

  
  int ii;
  for (ii = 0; ii < 6; ii++) {
     layer_add_child(dataLayer, text_layer_get_layer(dataInverterPT[ii]));
  }
    

// Create all the fields we will use on all screen layouts  
  int i;
  for (i =0; i < TITLE_INDEX; i++)
    {
    // Data
    text_layer_set_background_color(s_data_layer[i], GColorClear);
    text_layer_set_text_color(s_data_layer[i], normalTextColour);
    text_layer_set_text_alignment(s_data_layer[i], GTextAlignmentCenter);
    text_layer_set_overflow_mode(s_data_layer[i], GTextOverflowModeWordWrap);
    layer_add_child(dataLayer, text_layer_get_layer(s_data_layer[i]));
    
    //Title

    text_layer_set_background_color(s_data_title[i], GColorClear);
    text_layer_set_text_color(s_data_title[i], normalTitleColour);
    text_layer_set_text_alignment(s_data_title[i], GTextAlignmentCenter);
    
    if (i >= SIX_FIELD_INDEX && i <= SIX_FIELD_MAX) // Small title fonts on the 6 field layout
      {
      text_layer_set_font(s_data_layer[i], s_6_font);    
      text_layer_set_font(s_data_title[i], s_title_font);
    }
    else if (i >= TWO_FIELD_INDEX && i <= TWO_FIELD_MAX) // This is 2 fields
      {
      text_layer_set_font(s_data_layer[i], s_2_font); 
      text_layer_set_font(s_data_title[i], s_large_title_font);
    }

    else if (i >= FOUR_FIELD_INDEX && i <= FOUR_FIELD_MAX) // 4 field layout
      {
      text_layer_set_font(s_data_layer[i], s_4_font); 
      text_layer_set_font(s_data_title[i], s_medium_title_font);
    }
    else if (i >= THREE_FIELD_INDEX && i <= THREE_FIELD_MAX)
      {
      if (i == THREE_FIELD_INDEX) // First field is big
        {
        text_layer_set_font(s_data_layer[i], s_3_font); 
        text_layer_set_font(s_data_title[i], s_large_title_font);
      } else
        {
        text_layer_set_font(s_data_layer[i], s_4_font);    
        text_layer_set_font(s_data_title[i], s_medium_title_font);        
      }
    }      
   
    layer_add_child(titleLayer, text_layer_get_layer(s_data_title[i]));
    
  }

  // Add the heartbeat
 #ifdef PBL_COLOR
 layer_add_child(window_get_root_layer(window), text_layer_get_layer(flash));
 #else
 layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter));

  int ii;
  for (ii = 0; ii < 6; ii++) {
     layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(dataInverter[ii]));
  }

   layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(flash));
  #endif
    
//    layer_add_child(window_get_root_layer(window), text_layer_get_layer(messageOutline)); 
//    layer_add_child(window_get_root_layer(window), text_layer_get_layer(messageLayer)); 
  
  for (currentScreen = 0; screens[currentScreen].num_fields == 0; currentScreen++)
    ;


  s_padlockLayer = bitmap_layer_create(GRect(133, 3, 8, 11));
  s_res_padlock = gbitmap_create_with_resource(RESOURCE_ID_PADLOCK);
  bitmap_layer_set_bitmap(s_padlockLayer, s_res_padlock);
  layer_add_child(window_get_root_layer(window), (Layer *)s_padlockLayer);
  
  

  updatescreen(currentScreen,"277777"); // Make this too long & it crashes the Pebble!!!!!!
  doDataInvert(1);
}



static void main_window_unload(Window *window) {
  int i;
  for (i=0; i<TITLE_INDEX; i++)
    {
    text_layer_destroy(s_data_layer[i]);
    text_layer_destroy(s_data_title[i]);
  }
  
  layer_destroy(dataLayer);
  layer_destroy(titleLayer);
  text_layer_destroy(messageLayer);
  text_layer_destroy(s_data_layer[TITLE_INDEX]);  
  #ifdef PBL_COLOR
  text_layer_destroy(flash);
  #else
  inverter_layer_destroy(inverter);
  inverter_layer_destroy(flash);

  int jj;
  for (jj = 0; jj < 6; jj++) {
    #ifdef PBL_COLOR
    text_layer_destroy(dataInverterPT[jj]);
    #else
    inverter_layer_destroy(dataInverter[jj]);
    #endif
  }
  #endif
  
  bitmap_layer_destroy(s_padlockLayer);
  gbitmap_destroy(s_res_padlock);
  
  fonts_unload_custom_font(s_2_font);
  fonts_unload_custom_font(s_4_font);
  fonts_unload_custom_font(s_6_font);
}


void setField(int i,  bool negNum, char* value)
  {
  static PropertyAnimation *pa1[6] = {NULL}, *pa2[6] = {NULL}; //Arrays to cope with 6 fields

    static GSize textContent;
    static GRect gfrom, gto, gframe;
    TextLayer *flm = s_data_layer[screens[currentScreen].field_layer_map[i]];
    text_layer_set_text_alignment(flm, GTextAlignmentLeft);
    text_layer_set_text(flm, value); // This line only
    textContent = text_layer_get_content_size(flm);
    gframe = layer_get_frame((Layer *)flm);
    if (textContent.w > gframe.size.w) // Overflowed
      {
      if (1==1)
        {
        }

    }
}


//
// Returns true if it can find the current key in a large field on the screen. Used to work out how to format data
//

bool isBigField(int key)
  {
  int i;
  
  int nf = screens[currentScreen].num_fields;
  for (i = 0; i < nf; i++)
    if (keyTitles[screens[currentScreen].field_data_map[i]].key == key)
      break;
  if (i == nf) // Didn't find it
    return false;
  if (nf == 2 || /* nf == 4 || */ (nf == 3 && i == 0))
    return true;
  else
    return false;
}


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
}

static void click_config_provider(void *context) {
 
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  #ifndef PBL_COLOR
  window_set_fullscreen(s_main_window, true);
  #endif
  // Set click handlers
  window_set_click_config_provider(s_main_window, click_config_provider);
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);
  }

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

//
// Main
//
int main(void) {
  int i;
  init();
  app_event_loop();
  deinit();  
}


void updatescreen(int thisScreen, char *initialValue)
  {  
    static int lastScreen = -1;  // Remember where we came from 
    int i;

  if (thisScreen == -2) //Act as if we are starting from scratch -- there is no last screen
    {
    lastScreen = -1;
    thisScreen = 0;
  }
  
  if (lastScreen != -1)  // If we had a last screen, blank out fields
    {
    
    for (i=0; i< screens[lastScreen].num_fields; i++)
        {
        TextLayer *dl = s_data_layer[screens[lastScreen].field_layer_map[i]];
      
        text_layer_set_text(dl, "");
        text_layer_set_text(s_data_title[screens[lastScreen].field_layer_map[i]], "");
        text_layer_set_background_color(dl, GColorClear);
        text_layer_set_text_color(dl, GColorWhite);
      } 
  }
  
if (thisScreen != -1) // -1 if there is no screen to go to -- just blanking out lastScreen prior to default restore;
  {
    for (i=0; i<screens[thisScreen].num_fields; i++) // For now - put something in the fields
      {
      if (initialValue != NULL)
        //text_layer_set_text(s_data_layer[screens[thisScreen].field_layer_map[i]], initialValue);
        setField(i, false, initialValue);
    }
    
    // Set up titles
    for (i=0; i<screens[thisScreen].num_fields; i++)
      {
      if (screens[thisScreen].field_data_map[i] >= num_keytitles)
        screens[thisScreen].field_data_map[i] = 0;
      // XXX text_layer_set_background_color(s_data_title[screens[thisScreen].field_layer_map[i]], GColorClear);
      text_layer_set_text(s_data_title[screens[thisScreen].field_layer_map[i]], keyTitles[screens[thisScreen].field_data_map[i]].title);
    }
  lastScreen = thisScreen;
  
  int jj;
  for (jj = 0; jj < 6; jj++) {
    doDataRevert(jj);
  }
  static char buf[25];
  snprintf(buf, sizeof(buf), "StartLine    Screen %d", thisScreen + 1);
  text_layer_set_text(s_data_layer[TITLE_INDEX], buf);
  }
}
  

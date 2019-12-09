#define EFL_BETA_API_SUPPORT 1

#include <Elementary.h>
#include <Efl_Ui.h>
// Temporary workaround until Unified Text stops using Legacy classes internally
#include <efl_ui_text.eo.h>
#include <efl_text_interactive.eo.h>

static Efl_Ui_Text *_screen = NULL;            // Text widget showing current value
static int _prev_value = 0;                    // Value introduced before an operation (first operand
static int _curr_value = 0;                    // Value currently being introduced (second operand)
static char _operation = '=';                  // Last operation button pressed
static Eina_Bool _must_overwrite = EINA_FALSE; // Whether next number must be appended to current input
                                               // or overwrite it

// Quits the application
static void
_gui_quit_cb(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   efl_exit(0);
}

// Performs "operation" on "currValue" and "prevValue" and leaves result in "currValue"
static void
_operate()
{
   switch (_operation)
   {
      case '+':
         _curr_value += _prev_value;
         break;
      case '-':
         _curr_value = _prev_value - _curr_value;
         break;
      case '*':
         _curr_value *= _prev_value;
         break;
      case '/':
         _curr_value = _prev_value / _curr_value;
         break;
      default:
         break;
   }
}

// Called every time a button is pressed
static void
_button_pressed_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   char button = ((const char *)data)[0];
   // If it is a number, append it to current input (or replace it)
   if (button >= '0' && button <= '9')
     {
        char str[2] = { button, '\0' };
        if (_must_overwrite)
          {
             efl_text_set(_screen, "");
             _must_overwrite = EINA_FALSE;
          }
        Efl_Text_Cursor *cursor = efl_text_interactive_main_cursor_get(_screen);
        efl_text_cursor_text_insert(cursor, str);
     }
   else
     {
        char str[32] = "";
        switch (button)
        {
           case 'C':
              // Clear current input
              efl_text_set(_screen, "0");
              break;
           case '+':
           case '-':
           case '*':
           case '/':
           case '=':
              // If there was a pending operation, perform it
              if (_operation != '=')
                {
                   _operate();
                   snprintf(str, sizeof(str), "%d", _curr_value);
                   efl_text_set(_screen, str);
                }
              // Store this operation
              _operation = button;
              _must_overwrite = EINA_TRUE;
              _prev_value = _curr_value;
              break;
           default:
              break;
        }
     }
}

// Called every time the content of the screen changes
// We use it to sanitize input (remove heading zeros, for example)
// This makes more sense when the Text widget is editable, since the user
// is free to type anything.
static void
_screen_changed_cb(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   printf("The Text : %s\n",efl_text_get(_screen));
   char _text[32] = "";
   const char *str = efl_text_get(_screen);
   printf("%s -> ", str);
   int d;
   if ((strcmp(str, "") == 0) || (strcmp(str, "-") == 0))
     {
        snprintf(_text, sizeof(_text), "0");
     }
   else if (sscanf(str, "%d", &d) == 1)
     {
        snprintf(_text, sizeof(_text), "%d", d);
        _curr_value = d;
     }
   if (strncmp(_text, str, sizeof(_text)))
     {
        efl_event_callback_del(_screen, EFL_UI_TEXT_EVENT_CHANGED, _screen_changed_cb, NULL);
        efl_text_set(_screen, _text);
        efl_event_callback_add(_screen, EFL_UI_TEXT_EVENT_CHANGED, _screen_changed_cb, NULL);
     }
   printf("%s\n", _text);
}

// Creates an Efl.Ui.Button and positions it in the given position inside the table
// The button text is colored with "r, g, b"
// "text" is what is drawn on the button, which might be a multi-byte unicode string.
// "command" is a single-char id for the button.
static void
_button_add(Efl_Ui_Table *table, const char *text, const char *command, int posx, int posy, int r, int g, int b)
{
   Efl_Ui_Button *button =
      efl_add(EFL_UI_BUTTON_CLASS, table,
              efl_pack_table(table, efl_added, posx, posy, 1, 1),
              efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _button_pressed_cb, command));
   // Buttons can only have simple text (no font, styles or markup) but can swallow
   // any other object we want.
   // Therefore we create a more complex Efl_Ui_Text object and use it as content for the button.
   Efl_Ui_Text *label =
      efl_add(EFL_UI_TEXT_CLASS, button,
              efl_text_interactive_editable_set(efl_added, EINA_FALSE),
              efl_text_horizontal_align_set(efl_added, 0.5),
              efl_text_vertical_align_set(efl_added, 0.5),
              efl_gfx_color_set(efl_added, r, g, b, 255),
              efl_text_set(efl_added, text));
   efl_text_font_family_set(label, "Sans");
   efl_text_font_size_set(label, 36);
   efl_content_set(button, label);
}

// Creates the UI
static void
_gui_setup()
{
   Eo *win, *table;

   // The window
   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                 efl_text_set(efl_added, "EFL Calculator"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   // when the user clicks "close" on a window there is a request to delete
   efl_event_callback_add(win, EFL_UI_WIN_EVENT_DELETE_REQUEST, _gui_quit_cb, NULL);

   // The table is the main layout
   table = efl_add(EFL_UI_TABLE_CLASS, win,
                   efl_content_set(win, efl_added),
                   efl_pack_table_size_set(efl_added, 4, 5),
                   efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(300, 400)));

   // Create all buttons using the _button_add helper
   _button_add(table, "1", "1", 0, 3, 255, 255, 255);
   _button_add(table, "2", "2", 1, 3, 255, 255, 255);
   _button_add(table, "3", "3", 2, 3, 255, 255, 255);
   _button_add(table, "4", "4", 0, 2, 255, 255, 255);
   _button_add(table, "5", "5", 1, 2, 255, 255, 255);
   _button_add(table, "6", "6", 2, 2, 255, 255, 255);
   _button_add(table, "7", "7", 0, 1, 255, 255, 255);
   _button_add(table, "8", "8", 1, 1, 255, 255, 255);
   _button_add(table, "9", "9", 2, 1, 255, 255, 255);
   _button_add(table, "0", "0", 1, 4, 255, 255, 255);
   _button_add(table, "+", "+", 3, 1, 128, 128, 128);
   _button_add(table, "−", "-", 3, 2, 128, 128, 128);
   _button_add(table, "×", "*", 3, 3, 128, 128, 128);
   _button_add(table, "÷", "/", 3, 4, 128, 128, 128);
   _button_add(table, "=", "=", 2, 4, 128, 128, 128);
   _button_add(table, "C", "C", 0, 4,   0,   0,   0);

   // Create a big Efl.Ui.Text screen to display the current input
   _screen = efl_add(EFL_UI_TEXT_CLASS, table,
                     efl_text_set(efl_added, "0"),
                     efl_text_multiline_set(efl_added, EINA_FALSE),
                     efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                     efl_text_interactive_selection_allowed_set(efl_added, EINA_FALSE),
                     efl_pack_table(table, efl_added, 0, 0, 4, 1),
                     efl_text_horizontal_align_set(efl_added, 0.9),
                     efl_text_vertical_align_set(efl_added, 0.5),
                     efl_text_effect_type_set(efl_added, EFL_TEXT_STYLE_EFFECT_TYPE_GLOW),
                     efl_text_glow_color_set(efl_added, 128, 128, 128, 128),
                     efl_event_callback_add(efl_added, EFL_UI_TEXT_EVENT_CHANGED,
                                            _screen_changed_cb, NULL));
   efl_text_font_family_set(_screen, "Sans");
   efl_text_font_size_set(_screen, 48);
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   _gui_setup();
}
EFL_MAIN()

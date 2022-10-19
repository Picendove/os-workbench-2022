#include <game.h>

//***
#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names1[] = {
  AM_KEYS(KEYNAME)
};
// ***


// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  

  puts("Press any key to see its key code...\n");

  while (1) {
    AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode != AM_KEY_NONE && event.keydown) {
      puts("Key pressed: ");
      switch(*key_names1[event.keycode])
      {
        case 'W': 
          splash('W');
          break;
        case 'A': 
          splash('A');
          break;
        case 'S': 
          splash('S');
          break;
        case 'D': 
          splash('D');
          break;
        default: 
          print_key();
          puts("undefined behavior, try \"WSAD\" \n");
      } 
    }
  }
  return 0;
}

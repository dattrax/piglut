#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*displayCallback)(void *pg);
typedef bool (*keyboardCallback)(void *pg, char key);
typedef void (*initCallback)(void *pg);

typedef struct
{
   unsigned int width;
   unsigned int height;
   unsigned int panelWidth;
   unsigned int panelHeight;
} piglutDisplayConfig_t;

typedef struct
{
   unsigned int width;
   unsigned int height;
   unsigned int bpp;
} piglutWindowConfig_t;

void * piglutInit(int argc, char **argv);

void piglutTerm(void *pg);

/* initializes the window config to a default */
void piglutInitWindowConfig(piglutWindowConfig_t * wc);

int piglutInitWindowSize(void *pg,
                         piglutWindowConfig_t * wc);

int piglutGetDisplayConfig(void *pg,
                           piglutDisplayConfig_t * dc);

int piglutDisplayFunc(void *pg,
                      displayCallback display);

int piglutKeyboardFunc(void *pg,
                       keyboardCallback keyboard);

int piglutInitFunc(void *pg,
                   initCallback init);

int piglutMainLoop(void *pg);

int piglutSetUserData(void *pg, void * userData);

void * piglutGetUserData(void *pg);

#ifdef __cplusplus
}
#endif


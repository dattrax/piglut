
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <alloca.h>
#include <termios.h>

#include "bcm_host.h"

#include <EGL/egl.h>

#include "piglut.h"

typedef struct
{
   bool widthFromCmdLine;
   bool heightFromCmdLine;
   bool bppFromCmdLine;
   unsigned int width;
   unsigned int height;
   unsigned int panelWidth;
   unsigned int panelHeight;
   unsigned int bpp;

   /* callbacks */
   displayCallback displayCb;
   keyboardCallback keyboardCb;
   initCallback initCb;

   /* set by anything to terminate the main loop */
   bool terminate;

   /* EGL */
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;

   /* dispmax stuff */
   EGL_DISPMANX_WINDOW_T nativeWindow;
   DISPMANX_ELEMENT_HANDLE_T dispmanElement;
   DISPMANX_DISPLAY_HANDLE_T dispmanDisplay;
   DISPMANX_UPDATE_HANDLE_T dispmanUpdate;

   /* keyboard input */
   struct termios oldTerminalConfig;
   int peekCharacter;

   /* user data */
   void * userData;
} piglut_t;

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

void * piglutInit(int argc, char **argv)
{
   piglut_t * p = (piglut_t *)malloc(sizeof(piglut_t));
   if (p)
   {
      memset(p, 0, sizeof(piglut_t));

      /* TODO : command line parsing */
   }

   /* NULL on error */
   return (void *)p;
}

void piglutTerm(void *pg)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      /* TODO : find out what's required to terminate */
      p->dispmanUpdate = vc_dispmanx_update_start(0);
      vc_dispmanx_element_remove(p->dispmanDisplay, p->dispmanElement);
      vc_dispmanx_update_submit_sync(p->dispmanUpdate);

      vc_dispmanx_display_close(p->dispmanDisplay);

      bcm_host_deinit();

      /* makes sure that if anyone kept a reference, it's gone */
      memset(p, 0, sizeof(piglut_t));
      free(p);
   }
}

/* allows you to override options not specified at the command line */
int piglutInitWindowSize(void *pg,
                         unsigned int width,
                         unsigned int height,
                         unsigned int bpp)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      /* verify the new values prior to saving them */
      if (!p->widthFromCmdLine)
         width = MIN(width, MAX_WIDTH);

      if (!p->heightFromCmdLine)
         height = MIN(height, MAX_HEIGHT);

      if ((!p->bppFromCmdLine) && (bpp == 32))
         p->bpp = 32;
      else if ((!p->bppFromCmdLine) && (bpp == 24))
         p->bpp = 24;
      else if ((!p->bppFromCmdLine) && (bpp == 16))
         p->bpp = 16;
      else
      {
         errno = EINVAL;
         return -1;
      }

      p->width = width;
      p->height = height;

      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

int piglutDisplayFunc(void *pg,
                      displayCallback display)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      p->displayCb = display;
      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

int piglutKeyboardFunc(void *pg,
                       keyboardCallback keyboard)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      p->keyboardCb = keyboard;
      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

int piglutInitFunc(void *pg,
                   initCallback init)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      p->initCb = init;
      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

static void populateConfig(EGLint *p,
                           unsigned int bpp,
                           unsigned int depthSize,
                           unsigned int stencilSize,
                           unsigned int multisample)
{
   int i = 0;
   p[i++] = EGL_RED_SIZE;
   if (bpp == 16)
      p[i++] = 5;
   else if (bpp > 16)
      p[i++] = 8;
   else
      p[i++] = 0;

   p[i++] = EGL_GREEN_SIZE;
   if (bpp == 16)
      p[i++] = 6;
   else if (bpp > 16)
      p[i++] = 8;
   else
      p[i++] = 0;

   p[i++] = EGL_BLUE_SIZE;
   if (bpp == 16)
      p[i++] = 5;
   else if (bpp > 16)
      p[i++] = 8;
   else
      p[i++] = 0;

   p[i++] = EGL_ALPHA_SIZE;
   if (bpp == 16)
      p[i++] = 0;
   else if (bpp == 24)
      p[i++] = 0;
   else if (bpp == 32)
      p[i++] = 8;
   else
      p[i++] = 0;

   p[i++] = EGL_DEPTH_SIZE;
   p[i++] = depthSize;

   p[i++] = EGL_STENCIL_SIZE;
   p[i++] = stencilSize;

   if (multisample)
   {
      p[i++] = EGL_SAMPLE_BUFFERS;
      p[i++] = 1;
      p[i++] = EGL_SAMPLES;
      p[i++] = 4;
   }

   p[i++] = EGL_SURFACE_TYPE;
   p[i++] = EGL_WINDOW_BIT;

   p[i++] = EGL_RENDERABLE_TYPE;
   p[i++] = EGL_OPENGL_ES2_BIT;

   p[i++] = EGL_NONE;
}

static int kbhit(piglut_t * p)
{
   unsigned char ch;
   int nRead;
   struct termios term;

   /* character pending */
   if (p->peekCharacter != -1)
      return 1;

   tcgetattr(STDIN_FILENO, &term);
   term.c_cc[VMIN] = 0;
   tcsetattr(STDIN_FILENO, TCSANOW, &term);
   nRead = read(STDIN_FILENO, &ch, 1);
   term.c_cc[VMIN] = 1;
   tcsetattr(STDIN_FILENO, TCSANOW, &term);
   if (nRead == 1)
   {
      p->peekCharacter = ch;
      return 1;
   }
   return 0;
}

static int readch(piglut_t * p)
{
   char ch;

   /* already had a char from earlier, just return it */
   if (p->peekCharacter != -1)
   {
      ch = p->peekCharacter;
      p->peekCharacter = -1;
      return ch;
   }

   /* nothing pending, just read */
   read(0, &ch, 1);
   return ch;
}


int piglutMainLoop(void *pg)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      VC_RECT_T dstRect;
      VC_RECT_T srcRect;
      VC_DISPMANX_ALPHA_T layerAlpha;
      EGLint configAttributes[32];
      EGLint numberConfigs, selectedConfig;
      EGLConfig * eglConfigs;
      int i;
      struct termios newTerminalConfig;

      static const EGLint contextAttributes[] =
      {
         EGL_CONTEXT_CLIENT_VERSION, 2,
         EGL_NONE
      };

      bcm_host_init();

      /* setup dispmax */
      if (graphics_get_display_size(0 /* LCD */, &p->panelWidth, &p->panelHeight))
      {
         errno = ECONNREFUSED;
         return -1;
      }
      /* reclamp the state width and height to that of the display */
      p->width = MIN(p->width, p->panelWidth);
      p->height = MIN(p->height, p->panelHeight);

      dstRect.x = 0;
      dstRect.y = 0;
      dstRect.width = p->panelWidth;
      dstRect.height = p->panelHeight;

      srcRect.x = 0;
      srcRect.y = 0;
      srcRect.width = p->width << 16;
      srcRect.height = p->height << 16;

      /* this is nothing to do with the EGL window having alpha, but how its
         blended to the console underneath */
      layerAlpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
      layerAlpha.opacity = 255;
      layerAlpha.mask = 0;

      p->dispmanDisplay = vc_dispmanx_display_open(0 /* LCD */);

      /* this pairs with the vc_dispmanx_update_submit_sync() below,
         which applies the changes inbetween */
      p->dispmanUpdate = vc_dispmanx_update_start(0);

      p->dispmanElement = vc_dispmanx_element_add(p->dispmanUpdate,
                                                  p->dispmanDisplay,
                                                  0/*layer*/,
                                                  &dstRect,
                                                  0/*src*/,
                                                  &srcRect,
                                                  DISPMANX_PROTECTION_NONE,
                                                  &layerAlpha,
                                                  0/*clamp*/,
                                                  0/*transform*/);

      p->nativeWindow.element = p->dispmanElement;
      p->nativeWindow.width = p->width;
      p->nativeWindow.height = p->height;

      vc_dispmanx_update_submit_sync(p->dispmanUpdate);

      /* TODO : Add cleanup if failure */

      /* create an EGL context */
      p->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      if (p->display == EGL_NO_DISPLAY)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      /* initialize the EGL display connection */
      if (eglInitialize(p->display, NULL, NULL) == EGL_FALSE)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      /* TODO : add depth stencil config to the API */
      populateConfig(configAttributes, p->bpp, 15, 1, false);

      if (!eglGetConfigs(p->display, NULL, 0, &numberConfigs))
      {
         errno = ECONNREFUSED;
         return -1;
      }

      eglConfigs = (EGLConfig *)alloca(numberConfigs * sizeof(EGLConfig));

      if (!eglChooseConfig(p->display, configAttributes, eglConfigs, numberConfigs, &numberConfigs) || (numberConfigs == 0))
      {
         errno = ECONNREFUSED;
         return -1;
      }

      for (i = 0; i < numberConfigs; i++)
      {
         EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
         eglGetConfigAttrib(p->display, eglConfigs[i], EGL_RED_SIZE, &redSize);
         eglGetConfigAttrib(p->display, eglConfigs[i], EGL_GREEN_SIZE, &greenSize);
         eglGetConfigAttrib(p->display, eglConfigs[i], EGL_BLUE_SIZE, &blueSize);
         eglGetConfigAttrib(p->display, eglConfigs[i], EGL_ALPHA_SIZE, &alphaSize);
         eglGetConfigAttrib(p->display, eglConfigs[i], EGL_DEPTH_SIZE, &depthSize);

         if (p->bpp == (redSize + greenSize + blueSize + alphaSize))
            break;
      }
      selectedConfig = i;

      /* get an appropriate EGL frame buffer configuration */
      if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      /* create an EGL rendering context */
      p->context = eglCreateContext(p->display, eglConfigs[selectedConfig], EGL_NO_CONTEXT, contextAttributes);
      if (p->context == EGL_NO_CONTEXT)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      p->surface = eglCreateWindowSurface(p->display, eglConfigs[selectedConfig], &p->nativeWindow, NULL);
      if (p->surface == EGL_NO_SURFACE)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      /* connect the context to the surface */
      if (eglMakeCurrent(p->display, p->surface, p->surface, p->context) == EGL_FALSE)
      {
         errno = ECONNREFUSED;
         return -1;
      }

      /* this is called when GL is up, so suits texture loading, one time init, etc */
      if (p->initCb)
         p->initCb(pg);

      /* set the initial condition for kbhit & readch */
      p->peekCharacter = -1;

      tcgetattr(STDIN_FILENO, &p->oldTerminalConfig);
      newTerminalConfig = p->oldTerminalConfig;
      /* remove buffering and echo */
      newTerminalConfig.c_lflag &= ~(ICANON | ECHO | ISIG);
      newTerminalConfig.c_cc[VMIN] = 1;
      newTerminalConfig.c_cc[VTIME] = 0;
      tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);

      while (!p->terminate)
      {
         if (p->keyboardCb)
         {
            if (kbhit(p))
            {
               do
               {
                  /* returning true from the keyboard function will quit */
                  p->terminate = p->keyboardCb(pg, readch(p));
               } while (kbhit(p));
            }
         }
         if (p->displayCb)
            p->displayCb(pg);
      }

      /* return the keyboard to default handler state */
      tcsetattr(STDIN_FILENO, TCSANOW, &p->oldTerminalConfig);

      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

int piglutSetUserData(void *pg, void * userData)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
   {
      p->userData = userData;
      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}

void * piglutGetUserData(void *pg)
{
   piglut_t * p = (piglut_t *)pg;
   if (p)
      return p->userData;
   else
   {
      errno = EINVAL;
      return 0;
   }
}

int piglutGetDisplayConfig(void *pg,
                            piglutDisplayConfig_t * dc)
{
   piglut_t * p = (piglut_t *)pg;
   if (p && dc)
   {
      dc->width = p->width;
      dc->height = p->height;
      dc->panelWidth = p->panelWidth;
      dc->panelHeight = p->panelHeight;

      return 0;
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
}


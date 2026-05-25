#include "game.h"
#include <ace/generic/main.h>
#include <ace/managers/game.h>
#include <ace/managers/state.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include <ace/managers/copper.h>
#include <ace/utils/palette.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/font.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include <ace/generic/screen.h>

#include "gamelogic/intro/intro.h"
#include "support/gcc8_c_support.h"

// Global library base pointers required by Amiga OS and Bartman GCC
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

tView *view;
tVPort *vPort;
tSimpleBufferManager *buffer;
tState *introState;
tStateManager *stateManager;

void genericCreate(void)
{
    // Initialize the ACE State Manager
    stateManager = stateManagerCreate();

    // Initialize key manager so keyboard input can be processed
    keyCreate();

    // Setup PAL display View: force 320x256 regardless of host hardware
    view = viewCreate(0,
                      TAG_VIEW_GLOBAL_PALETTE, 1,
                      TAG_VIEW_WINDOW_WIDTH, SCREEN_PAL_WIDTH,
                      TAG_VIEW_WINDOW_HEIGHT, SCREEN_PAL_HEIGHT,
                      TAG_VIEW_WINDOW_START_Y, SCREEN_PAL_YOFFSET,
                      TAG_DONE);

    // Create standard Viewport: 320x256, 5 bitplanes
    vPort = vPortCreate(0,
                        TAG_VPORT_VIEW, view,
                        TAG_VPORT_BPP, 5,
                        TAG_VPORT_WIDTH, SCREEN_PAL_WIDTH,
                        TAG_VPORT_HEIGHT, SCREEN_PAL_HEIGHT,
                        TAG_DONE);

    // Create double-buffered SimpleBuffer manager
    buffer = simpleBufferCreate(0,
                                TAG_SIMPLEBUFFER_VPORT, vPort,
                                TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
                                TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                TAG_DONE);

    // Load the view to the display hardware immediately
    viewLoad(view);

    KPrintF("Starting game");

    // Build and push the intro state
    introState = stateCreate(introCreate, introLoop, introDestroy, 0, 0);
    statePush(stateManager, introState);

    // Disable OS multitasking and take full hardware control during gameplay
    systemUnuse();
}

void genericProcess(void)
{
    // Update key states for this frame before any state logic reads them
    keyProcess();

    // Process active state callbacks (e.g. introLoop)
    stateProcess(stateManager);

    // Process hardware rendering and double-buffering updates
    viewProcessManagers(view);
    copProcessBlocks();
    vPortWaitForEnd(vPort);
}

void genericDestroy(void)
{
    // Re-enable OS multitasking before shutting down the game
    systemUse();

    // Free allocated framework engines and objects
    // stateManagerDestroy handles cbDestroy callbacks and internal state cleanup;
    // do NOT call stateDestroy separately as it would double-free the state.
    stateManagerDestroy(stateManager);
    viewDestroy(view);
    keyDestroy();
}

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

#include "gamelogic/intro/intro.h"

// Global library base pointers required by Amiga OS and Bartman GCC
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

tView *g_pView;
tVPort *g_pVPort;
tSimpleBufferManager *g_pBuffer;
tState *g_pIntroState;
tStateManager *g_pStateManager;

void genericCreate(void)
{
    // Initialize the ACE State Manager
    g_pStateManager = stateManagerCreate();

    // Setup basic display View
    g_pView = viewCreate(0,
                         TAG_VIEW_GLOBAL_PALETTE, 1,
                         TAG_DONE);

    // Create standard Viewport: 320x256, 5 bitplanes
    g_pVPort = vPortCreate(0,
                           TAG_VPORT_VIEW, g_pView,
                           TAG_VPORT_BPP, 5,
                           TAG_DONE);

    // Create double-buffered SimpleBuffer manager
    g_pBuffer = simpleBufferCreate(0,
                                   TAG_SIMPLEBUFFER_VPORT, g_pVPort,
                                   TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
                                   TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                   TAG_DONE);

    // Load the view to the display hardware immediately
    viewLoad(g_pView);

    // Build and push the intro state
    g_pIntroState = stateCreate(introCreate, introLoop, introDestroy, 0, 0);
    statePush(g_pStateManager, g_pIntroState);
}

void genericProcess(void)
{
    // Process active state callbacks (e.g. introLoop)
    stateProcess(g_pStateManager);

    // Process hardware rendering and double-buffering updates
    viewProcessManagers(g_pView);
    copProcessBlocks();
    vPortWaitForEnd(g_pVPort);
}

void genericDestroy(void)
{
    // Free allocated framework engines and objects
    stateManagerDestroy(g_pStateManager);
    stateDestroy(g_pIntroState);
    viewDestroy(g_pView);
}

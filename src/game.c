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

const UWORD WINDOW_SCREEN_WIDTH = 320;  // SCREEN_PAL_WIDTH;
const UWORD WINDOW_SCREEN_HEIGHT = 256; // SCREEN_PAL_HEIGHT;
const UWORD WINDOW_SCREEN_BPP = 4;

tView *g_pView;
tVPort *g_pVPort;
tSimpleBufferManager *g_pBufferManager;
tFont *g_pFont;
tBitMap *g_pBackgroundBitMap;

tStateManager *g_pStateMachineGame;
static tState *introState;

// Called automatically by generic/main.h after its internal hardware setup
void genericCreate(void)
{
}

// Called automatically by generic/main.h every frame
void genericProcess(void)
{
    keyProcess();

    stateProcess(g_pStateMachineGame);
    copProcessBlocks();

    vPortWaitForEnd(g_pVPort); // Modern replacement for WaitTOF()
}

// Called automatically by generic/main.h when the loop ends
void genericDestroy(void)
{
    stateManagerDestroy(g_pStateMachineGame);

    if (g_pFont)
        fontDestroy(g_pFont);
    if (g_pBackgroundBitMap)
        bitmapDestroy(g_pBackgroundBitMap);

    viewLoad(0);
    viewDestroy(g_pView); // Also automatically destroys g_pVPort and g_pBufferManager

    keyDestroy();
}
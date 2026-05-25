#include "game.h"
#include <ace/generic/main.h>
#include <ace/managers/game.h>
#include <ace/managers/state.h>
#include <ace/managers/key.h>

// Global library base pointers required by Amiga OS and Bartman GCC
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

static tState *mainState;

void cbInit(void)
{
    keyUse(KEY_ESCAPE);
}

void cbLoop(void)
{
    if (keyCheck(KEY_ESCAPE))
    {
        // Flags the generic/main.h condition to break its loop
        gameClose();
    }
}

void cbDestroy(void)
{
    // Empty for now
}

// Called automatically by generic/main.h after its internal hardware setup
void genericCreate(void)
{
    mainState = stateCreate(cbInit, cbLoop, cbDestroy, 0, 0);
    statePush(g_pStateMachineGame, mainState);
}

// Called automatically by generic/main.h every frame
void genericProcess(void)
{
    // Empty! generic/main.h automatically processes keys and the state machine
}

// Called automatically by generic/main.h when the loop ends
void genericDestroy(void)
{
    // Empty! generic/main.h cleans up the state machine and hardware automatically
}
#include "intro.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>

void introCreate(void)
{
    keyUse(KEY_ESCAPE);
}

void introLoop(void)
{
    if (keyCheck(KEY_ESCAPE))
    {
        gameExit();
    }
}

void introDestroy(void)
{
    // Empty for now
}
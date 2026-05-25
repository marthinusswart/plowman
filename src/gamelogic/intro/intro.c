#include "intro.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/log.h>
#include <ace/managers/ptplayer.h>

static tPtplayerMod *mod;

void introCreate(void)
{
    // Init CIA-B interrupt for the ProTracker player in PAL mode
    ptplayerCreate(1);

    // Load the intro music module from disk
    mod = ptplayerModCreateFromPath("sound/mod/coal-prelude.mod");
    if (!mod) {
        logWrite("ERR: Failed to load sound/mod/coal-prelude.mod\n");
        return;
    }

    // Queue the mod for playback from the beginning, no external sample pack
    ptplayerLoadMod(mod, NULL, 0);

    // Loop the song indefinitely
    ptplayerConfigureSongRepeat(1, NULL);

    // Start playback
    ptplayerEnableMusic(1);
}

void introLoop(void)
{
    // Pump the ptplayer state machine each frame
    ptplayerProcess();

    // keyUse returns 1 only on the first frame the key transitions to pressed,
    // then marks it USED so it won't re-fire while held down.
    if (keyUse(KEY_ESCAPE))
    {
        gameExit();
    }
}

void introDestroy(void)
{
    // Stop music and release all ptplayer resources
    ptplayerEnableMusic(0);
    ptplayerStop();
    ptplayerDestroy();
    if (mod) {
        ptplayerModDestroy(mod);
        mod = NULL;
    }
}
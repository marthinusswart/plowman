#include "intro.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/log.h>
#include <ace/managers/ptplayer.h>
#include <ace/managers/copper.h>
#include <ace/managers/system.h>

#include "../../game.h"
#include <ace/utils/bitmap.h>
#include <ace/utils/palette.h>
#include <ace/managers/blit.h>
#include <ace/utils/disk_file.h>
#include <ace/utils/custom.h>

static tPtplayerMod *mod;
static tPtplayerSfx *sfx;
static UWORD frameCounter = 0;
static tCopBlock *palBlock = NULL;

void introCreate(void)
{
    // Re-enable OS once for efficient batched resource loading
    systemUse();

    frameCounter = 0;

    // 1. Load the raw tiles palette (64 bytes = 32 colors) directly into vPort->pPalette
    tFile *palFile = diskFileOpen("bpl/plowman_tiles.pal", DISK_FILE_MODE_READ, 1);
    if (palFile) {
        fileRead(palFile, vPort->pPalette, 32 * sizeof(UWORD));
        fileClose(palFile);
        
        // Create custom copper block at wait position (0, 0) for 32 MOVE instructions
        palBlock = copBlockCreate(view->pCopList, 32, 0, 0);
        if (palBlock) {
            for (UWORD i = 0; i < 32; ++i) {
                copMove(view->pCopList, palBlock, &g_pCustom->color[i], vPort->pPalette[i]);
            }
        } else {
            logWrite("ERR: Failed to create copper block for palette\n");
        }
    } else {
        logWrite("ERR: Failed to open bpl/plowman_tiles.pal\n");
    }

    // 2. Load the raw headerless splash background bitmap (320x239, 5 bitplanes)
    tBitMap *splash = bitmapCreate(320, 239, 5, 0);
    if (splash) {
        tFile *bplFile = diskFileOpen("bpl/plowman_splash.bpl", DISK_FILE_MODE_READ, 1);
        if (bplFile) {
            // Read raw planar data: 320x239 @ 5BPP is exactly (320/8)*239 = 9560 bytes per plane
            for (UBYTE i = 0; i < 5; ++i) {
                fileRead(bplFile, splash->Planes[i], 9560);
            }
            fileClose(bplFile);
            
            // Blit the 320x239 splash graphic onto both front and back buffers
            blitCopy(splash, 0, 0, buffer->pBack, 0, 0, 320, 239, MINTERM_COPY);
            blitCopy(splash, 0, 0, buffer->pFront, 0, 0, 320, 239, MINTERM_COPY);
        } else {
            logWrite("ERR: Failed to open bpl/plowman_splash.bpl\n");
        }
        bitmapDestroy(splash);
    } else {
        logWrite("ERR: Failed to allocate splash bitmap\n");
    }

    // 3. Force-load the view again to apply the new copper list pointers and updated palette.
    // This displays the splash screen instantly before we begin loading large audio files!
    viewLoad(view);

    // 4. Init CIA-B interrupt for the ProTracker player in PAL mode
    ptplayerCreate(1);

    // Load the intro music module from disk
    mod = ptplayerModCreateFromPath("sound/mod/coal-prelude.mod");
    if (!mod) {
        logWrite("ERR: Failed to load sound/mod/coal-prelude.mod\n");
    } else {
        // Queue the mod for playback from the beginning, no external sample pack
        ptplayerLoadMod(mod, NULL, 0);

        // Loop the song indefinitely
        ptplayerConfigureSongRepeat(1, NULL);

        // Start playback
        ptplayerEnableMusic(1);
    }

    // Load the HUD message sound effect (using Chip RAM by passing 0 for isFast)
    sfx = ptplayerSfxCreateFromPath("sound/sfx/hud_msg.sfx", 0);
    if (!sfx) {
        logWrite("ERR: Failed to load sound/sfx/hud_msg.sfx\n");
    }

    // Disable OS multitasking and take full hardware control during gameplay
    systemUnuse();
}

void introLoop(void)
{
    // Pump the ptplayer state machine each frame
    ptplayerProcess();

    // Play HUD message sound effect every 5 seconds (250 frames in PAL mode)
    frameCounter++;
    if (frameCounter >= 250)
    {
        if (sfx)
        {
            ptplayerSfxPlay(sfx, PTPLAYER_SFX_CHANNEL_ANY, 64, 1);
        }
        frameCounter = 0;
    }

    // keyUse returns 1 only on the first frame the key transitions to pressed,
    // then marks it USED so it won't re-fire while held down.
    if (keyUse(KEY_ESCAPE))
    {
        gameExit();
    }
}

void introDestroy(void)
{
    // Re-enable OS for clean destruction of all audio and copper resources
    systemUse();

    // Stop music and release all ptplayer resources
    ptplayerEnableMusic(0);
    ptplayerStop();
    ptplayerDestroy();
    if (mod) {
        ptplayerModDestroy(mod);
        mod = NULL;
    }
    if (sfx) {
        ptplayerSfxDestroy(sfx);
        sfx = NULL;
    }

    // Clean up copper block
    if (palBlock) {
        copBlockDestroy(view->pCopList, palBlock);
        palBlock = NULL;
    }

    // Disable OS
    systemUnuse();
}
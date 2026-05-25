#ifndef _GAME_H_
#define _GAME_H_

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include <ace/managers/state.h>

extern tView *g_pView;
extern tVPort *g_pVPort;
extern tSimpleBufferManager *g_pBuffer;
extern tStateManager *g_pStateManager;

#endif // _GAME_H_
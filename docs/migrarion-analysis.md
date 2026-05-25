Roadmap: Migrating Amiga Pac-Man to Full ACE Framework ArchitectureThis document provides a comprehensive analysis and step-by-step architectural roadmap for migrating the amiga-pacman codebase from its current hybrid structure (mixing direct Amiga hardware registers, OS calls, and selected ACE functions) to a fully compliant, native Amiga C Engine (ACE) framework architecture.1. Architectural Gap AnalysisCurrent Hybrid ArchitectureCurrently, main.c acts as a monolithic controller that manually handles hardware initialization, custom copper list generation, double-buffering management, state history tracking, and low-level interrupt registers.+------------------------------------------------------------+
| main.c Loop |
| - Process Inputs (keyCheck) |
| - Logic & Pellet Check |
| - Custom Clear Phase (Restore backgrounds manually) |
| - Custom Draw Phase (Draw entities via custom routines) |
| - WaitVbl() & Pointer swap (doubleBufferUpdates) |
+------------------------------------------------------------+
| | |
v v v
+---------------+ +---------------+ +----------------------+
| Amiga Custom | | OS Libraries | | Selected ACE Units |
| Registers | | (graphics/dos)| | (system, key, blit) |
+---------------+ +---------------+ +----------------------+
Proposed Native ACE ArchitectureBy migrating fully to ACE, the monolithic controller is replaced with an event-driven loop overseen by the Game State Manager. Drawing, history tracking, copper management, and screen splitting are delegated to the Viewport and Bob managers. +--------------------------+
| ACE Engine Core |
| (gameProcess) |
+--------------------------+
|
v
+--------------------------+
| State Manager (sState) |
| - cbCreate / cbLoop |
+--------------------------+
|
+---------------------+---------------------+
v v
+------------------+ +------------------+
| Graphics Engine | | Object Engine |
| - tView / tVPort | | - tBobSource |
| - SimpleBuffer | | - tBobManager |
+------------------+ +------------------+ 2. Key Migration PillarsPillar A: The Game Loop & State SystemCurrent State: A classic procedural while(TRUE) loop in main.c with direct keystroke handling, manually waiting for the vertical blank (WaitVbl()), and manual buffer pointer updates.Target State: Driven entirely by gameProcess() utilizing state wrappers (tState).Why: This frees the CPU from manually synchronizing frame boundaries, protects the system from race conditions, and makes introducing new game screens (e.g., Title Screen, High Score table, Cutscenes) as simple as pushing and popping state structures.Pillar B: Unified Viewports & Hardware Copper ManagementCurrent State: Custom allocation of a $1024$-byte block for a copper list, manually building copper instructions, writing bitplane pointers directly, and executing custom hardware pokes (cop1lc = ...).Target State: Managed by ACE tView and tVPort structures, with double-buffering handled by tSimpleBufferManager.Why: Direct copper manipulations risk breaking system stability on varying Amiga models (A500 vs A1200 / ECS vs AGA). ACE's Viewport manager automatically abstracts copper instructions, ensuring clean, hardware-compliant raster splits and interleaved bitplane handling.Pillar C: Automatic Blitter Objects (Bobs)Current State: Manual object structures tracking dual-buffer positions (lastPosition[NUM_ENTITIES][2]) to perform a hand-rolled "restore background $\rightarrow$ blit mask $\rightarrow$ draw sprite" cycle.Target State: Fully automated using ACE's Bob Manager (tBob and tBobSource).Why: Hand-rolling double-buffered Bob drawing requires maintaining offset histories for every object. The ACE Bob manager automates background backup, clean restoration, dirty-rect tracking, and masked blitting in optimized assembly.3. Detailed Refactoring Implementation1. Refactoring Game Flow (The State Manager)Rather than letting main.c hold all states globally, we configure state callbacks. This separates active gameplay logic from setup routines.#include <ace/managers/game.h>
#include <ace/managers/state.h>

// Forward declarations of state callbacks
static void gameCreate(void);
static void gameLoop(void);
static void gameDestroy(void);

static tState \*g_pGameState;

int main(void) {
// 1. Initialize ACE core engine (replaces systemCreate, systemUnuse, warpmode, etc.)
gameCreate(setupEnvironment, teardownEnvironment);

    // 2. Build the active gameplay state
    g_pGameState = stateCreate(gameCreate, gameLoop, gameDestroy, NULL, NULL, NULL);
    statePush(g_pGameState);

    // 3. Hand control to the ACE orchestrator loop
    gameProcess();

    // 4. Free allocated framework engines on exit
    gameDestroy();
    return 0;

} 2. Transitioning Graphics Setup to ViewportsLet's replace manual buffer setups (setupBuffers, setupCopper) with the native Viewport system. For Pac-Man, a single standard viewport is sufficient.#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>

tView *g_pView;
tVPort *g_pVPort;
tSimpleBufferManager \*g_pBuffer;

static void setupGraphics(void) {
// 1. Create a base system view
g_pView = viewCreate(0);

    // 2. Create a standard Viewport: 320x256, 5 bitplanes (32 colors)
    g_pVPort = vportCreate(g_pView, 320, 256, 5, 0);

    // 3. Create a double-buffered simple buffer manager
    g_pBuffer = simpleBufferCreate(g_pVPort, 320, 256, BMF_CLEAR);

    // 4. Apply palette
    paletteLoad("pal/pacman_tiles.pal", g_pVPort->pPalette, 32);

    // 5. Instantly generate copper and load the view to the display hardware
    viewLoad(g_pView);

} 3. Migrating to Native ACE BobsInstead of your manually processed bobUpdates and backgroundUpdates, we wrap each actor (Pacman, Ghosts) into standard Bob structures.Step 1: Initialize Bob Sources and Instances#include <ace/managers/bob.h>

tBobSource *g_pEntitySource; // Single shared tilesheet source
tBob *g_pPacmanBob;
tBob \*g_pRedGhostBob;

void setupGameEntities(void) {
// Load your bitplane sheet as a Bob Source
// Parameters: bitmap source, mask source, width, height, frame count
g_pEntitySource = bobSourceCreate(tPacmanTiles, (const UBYTE \*)pacman_tiles_mask, 16, 16, 16);

    // Create tracking instances pointing to the source frame
    g_pPacmanBob = bobCreate(g_pEntitySource, 16, 16);
    g_pRedGhostBob = bobCreate(g_pEntitySource, 16, 16);

    // Push the objects onto the active draw queue
    bobPush(g_pPacmanBob, g_pBuffer);
    bobPush(g_pRedGhostBob, g_pBuffer);

}
Step 2: Automated Rendering LoopYour active update loop simply moves the coordinates of the Bobs and makes a single call to trigger clear, update, and drawing steps.static void gameLoop(void) {
// 1. Logic and coordinate shifts
processInputs();
ghostUpdates();

    // 2. Set Bob coordinates (ACE remembers where they were to auto-restore backgrounds)
    bobMove(g_pPacmanBob, pacman->x, pacman->y);
    bobMove(g_pRedGhostBob, redGhost->x, redGhost->y);

    // 3. Single assembly-optimized call:
    //    - Restores old backgrounds on the back buffer
    //    - Backs up new backgrounds under current positions
    //    - Blits sprites into place on the back buffer
    bobUpdateAll(g_pBuffer);

    // 4. Swap display planes at VBlank
    gameSwapBuffers(g_pBuffer);

} 4. Native Input ProcessingReplace standard keyboard checks with the ACE Input system, which integrates seamlessly with joystick support (joy.h).#include <ace/managers/joy.h>
#include <ace/managers/key.h>

static void processInputs(void) {
// Check key states cleanly via ACE keys
if (keyCheck(KEY_ESCAPE)) {
gameClose(); // Gracefully stops gameProcess() execution loop
return;
}

    // Support both Joystick Port 1 and WASD/Arrow Keys automatically
    UBYTE joyState = joyGetState(JOY_PORT_1);

    if (joyState & JOY_LEFT || keyCheck(KEY_A)) {
        pacman->movePacman(pacman, LEFT);
    } else if (joyState & JOY_RIGHT || keyCheck(KEY_D)) {
        pacman->movePacman(pacman, RIGHT);
    } else if (joyState & JOY_UP || keyCheck(KEY_W)) {
        pacman->movePacman(pacman, UP);
    } else if (joyState & JOY_DOWN || keyCheck(KEY_S)) {
        pacman->movePacman(pacman, DOWN);
    }

} 4. Resource Allocation & Destruction MappingYour current custom teardown requires a careful 16-point sequence to prevent guru meditation crashes. When fully migrating to ACE, resource destruction scales down cleanly.[gameDestroy()]
|
+--> [stateDestroy()] ---> Free custom entities, positions, and lists.
|
+--> [viewDestroy()] ----> Automatically frees viewports, sub-buffers,
| associated bitmaps, and hardware Copper lists.
|
+--> [bobDestroy()] -----> Automatically cleans up all Bobs and BobSources.
|
+--> Return to OS -------> Restores ADKCON, DMACON, system interrupts, and
multitasking without direct assembly register pokes. 5. Step-by-Step Migration RoadmapTo transition your code safely without breaking gameplay functionality, follow this incremental development path:+-------------------------------------------------------+
| STEP 1: Implement State Manager & System Core Loop |
| - Implement gameCreate/gameProcess structure |
| - Keep existing manual drawing routines in cbLoop |
+-------------------------------------------------------+
|
v
+-------------------------------------------------------+
| STEP 2: Migrate to Viewports & SimpleBuffer |
| - Delete setupCopper and manual copper allocations |
| - Replace tScreenBuffers with SimpleBufferManager |
+-------------------------------------------------------+
|
v
+-------------------------------------------------------+
| STEP 3: Wrap Entities in tBob Structures |
| - Build g_pEntitySource from pacman_tiles |
| - Delete lastPosition tracker arrays |
| - Delete backgroundUpdates() and manual blitCopyMask |
+-------------------------------------------------------+
|
v
+-------------------------------------------------------+
| STEP 4: Integrate Native Audio & Cleanup Hooks |
| - Transition music routines to PTPlayer (ptplayer.h) |
| - Remove custom assembly interrupt setups |
+-------------------------------------------------------+ 6. Deep Dive: Map & Entity Collision Detection After MigrationA major point of clarity during this migration is understanding where game logic ends and engine rendering begins.Will your coded collision logic still be used?Yes, absolutely. Amiga C Engine (ACE) is a hardware abstraction, state control, and rendering platform. It does not include a high-level game physics engine. Mechanics such as Pac-Man staying inside lanes, turning only at tile intersections, wrapping around side-tunnels, and ghost-target tracking are game-specific algorithms that remain your responsibility.The arithmetic you wrote for map indexing is highly efficient for the Amiga’s $7.16\text{ MHz}$ Motorola 68000 CPU and should be preserved.How Map Collision Works Post-MigrationCurrently, your tile calculations utilize direct grid division (such as centerX >> 4 to divide by $16$ pixel tiles):int tileCol = centerX >> 4;
int tileRow = centerY >> 4;
int tileIndex = tileRow \* 20 + tileCol;
After migrating, you have two architecture choices for resolving these coordinate queries, depending on how your graphics are configured.Method A: Lightweight RAM Lookup Arrays (Recommended)This method keeps your custom 1D/2D structural arrays (e.g., currentStageMap or pelletsOnMap) in system RAM.Why: Memory reads on the 68000 CPU directly to flat RAM blocks are incredibly fast (taking only a few clock cycles).Integration: Your collision routines remain completely untouched. You compute the tile coordinates using your bit-shifted positions, check if RAMStageMap[tileIndex] represents a wall block, and constrain movement accordingly.Method B: Direct ACE Tilebuffer QueriesIf you choose to use ACE's native Tile Buffer Manager (tilebuffer.h / tTileBuffer) to manage your stage maps dynamically instead of loading a static flat background bitplane, you can query active tile properties dynamically from the engine.How: Use the framework's direct lookup function:UWORD tileType = tileBufferGetTile(g_pTileBuffer, tileCol, tileRow);
Why: This is highly useful if you want to implement destructible walls, opening doors, or dynamic obstacles, as the tile structure in memory matches what is actively displayed on screen.Designing High-Performance Grid Lane AlignmentIn arcade Pac-Man, characters can only change directions if they are perfectly aligned with the $16 \times 16$ tile grid lanes. If a player presses "UP" while Pac-Man is in the middle of a horizontal corridor, the direction change must be buffered until Pac-Man hits the center of an intersection tile.The following optimized implementation blueprint integrates your input processing with a grid alignment safety check:#define TILE_SIZE 16
#define LANE_THRESHOLD 1 // Permitted pixel alignment wiggle room

typedef struct {
int x;
int y;
UBYTE currentDir;
UBYTE bufferedDir;
} tPacman;

// Check if Pac-Man is centered inside a 16x16 tile lane
UBYTE isAlignedWithGrid(tPacman \*pPacman) {
// A modulo check determines if coordinates are multiples of 16
return ((pPacman->x % TILE_SIZE) == 0 && (pPacman->y % TILE_SIZE) == 0);
}

// Predict whether a direction check is blocked by a wall
UBYTE isPathClear(tPacman *pPacman, UBYTE direction, UBYTE *mapGrid) {
int nextCol = pPacman->x >> 4;
int nextRow = pPacman->y >> 4;

    switch (direction) {
        case LEFT:  nextCol--; break;
        case RIGHT: nextCol++; break;
        case UP:    nextRow--; break;
        case DOWN:  nextRow++; break;
    }

    // Avoid out of bound indexing
    if (nextCol < 0 || nextCol >= 20 || nextRow < 0 || nextRow >= 16) {
        return 0; // Outer boundaries/tunnel wraps
    }

    int nextTileIdx = (nextRow * 20) + nextCol;

    // Check if the coordinate index is a solid wall tile (e.g. tile code >= WALL_START)
    return (mapGrid[nextTileIdx] == EMPTY_TILE || mapGrid[nextTileIdx] == PELLET_TILE);

}

void updatePacmanMovement(tPacman *pPacman, UBYTE *mapGrid) {
// 1. If a direction transition is pending and we reach a grid lane crossing
if (pPacman->bufferedDir != pPacman->currentDir && isAlignedWithGrid(pPacman)) {
if (isPathClear(pPacman, pPacman->bufferedDir, mapGrid)) {
pPacman->currentDir = pPacman->bufferedDir; // Successfully execute turning swap!
}
}

    // 2. Advance coordinates if the path ahead is clear
    if (isPathClear(pPacman, pPacman->currentDir, mapGrid) || !isAlignedWithGrid(pPacman)) {
        switch (pPacman->currentDir) {
            case LEFT:  pPacman->x--; break;
            case RIGHT: pPacman->x++; break;
            case UP:    pPacman->y--; break;
            case DOWN:  pPacman->y++; break;
        }
    }

}
Key Takeaways for Your RefactoringZero Logic Lost: Your grid-coordinate transformations (col = x >> 4) and mapping buffers are preserved entirely.Simplified Entity-to-Entity Collisions: Collisions between Pac-Man and the ghosts remain simple coordinate proximity tests (abs(pacman->x - ghost->x) < 12 && abs(pacman->y - ghost->y) < 12). ACE's Bob instances expose clean tBob position bounds (x and y) that you can feed directly into your collision routines.7. Deep Dive: Pac-Man Animation Frame Management After MigrationUnderstanding how Pac-Man animates in your current codebase vs. how it transitions into ACE requires looking at how raw display planes are manipulated in Chip RAM.How Animation Works Currently in your CodeIn your hybrid architecture, main.c relies on asynchronous frame pulses and manual pixel offsets inside your drawing routine:The Timer Pulse (bobPulseCheck): At the end of every frame pass (following VBlank), bobPulseCheck(pacman) is invoked. This updates the player entity's local tick counters and increments an frame index variable (e.g., pacman->frame cycles $0 \rightarrow 1 \rightarrow 2 \rightarrow 3$).Direct Blitter Offsets (bobUpdates): Your bobUpdates() function manually calculates exactly which $16\times16$ sub-rectangle of tPacmanTiles contains the desired graphic. It computes horizontal and vertical bitplane offsets based on both pacman->direction and pacman->frame.The Blit Operation: It finally invokes blitCopyMask(), copying that offset region out of the raw bitplanes directly onto the back display buffer.This means you are manually calculating offset math on the CPU and driving low-level hardware blitter registers for every actor, every single frame.Will ACE still use your raw animation logic?Yes and No.Yes, you keep your high-level timing and directional math: The logic in bobPulseCheck that determines when a frame should change and which direction Pac-Man is facing is purely game-specific and is preserved $100\%$.No, you do not update the tBitMap of the Bob: In ACE, you do not manually manipulate the source bitmap pointers, calculate pixel coordinates, or perform direct blitter pokes. Modifying individual tBitMap pointers dynamically is highly inefficient on the 68000 CPU and causes flickering and double-buffering timing glitches.How Animation Works in ACE (tBobSource vs tBob)ACE cleanly decouples graphic storage from object instances via a shared sheet architecture:tBobSource (The Sheet Atlas): This is a single, read-only structure in Chip RAM containing your entire compilation of sprites (e.g., pacman_tiles.bpl). When you invoke bobSourceCreate(), you inform ACE how many frame slots exist in the sheet and their dimensions (e.g., $16 \times 16$ pixels, $16$ total slots).tBob (The Active Instance): This represents a single actor on the screen. It is incredibly lightweight, containing only coordinates and a frame slot index (uwFrame). It does not contain its own bitmap structure.To animate a character, your loop simply assigns a calculated frame number to the Bob, and ACE takes care of the rest:[Your Logic Loop] ----> (Calculates frameIndex) ----> [bobSetFrame(g_pPacmanBob, frameIndex)]
|
v
[ACE Render Phase] <---- (Auto-calculates srcX/srcY offsets) <-----+
Mapping Directions and Frames to 1D IndicesTo keep your sprite sheets organized, lay out your 16 frames in pacman_tiles.bpl as a sequential grid. For example:Frames 0–3: Pac-Man moving RIGHT (Closed, Semi-Open, Open, Semi-Open)Frames 4–7: Pac-Man moving LEFTFrames 8–11: Pac-Man moving UPFrames 12–15: Pac-Man moving DOWNYour movement loop can map the current direction and animation sequence animFrame to a single linear frame index using the following formula:$$frameIndex = (\text{direction} \times \text{frames\_per\_direction}) + \text{animFrame}$$Code Blueprint: Modernized ACE Animation LogicBelow is a complete implementation blueprint for updating Pac-Man's animation state after migrating to the full framework:#include <ace/managers/bob.h>
#include "player/pacman.h"

#define ANIM_DELAY 4 // VBlank frames to wait before toggling mouth animation
#define FRAMES_PER_DIR 4 // Animation frames allocated per direction index

// Setup initial state on game initialization
void initPacmanAnimation(tBobSource *pSharedSource, pSimpleBufferManager *pBuffer) {
// 1. Create Pac-Man Bob instance pointing to the shared source atlas
g_pPacmanBob = bobCreate(pSharedSource, 16, 16);

    // 2. Add to active double-buffer render pipeline
    bobPush(g_pPacmanBob, pBuffer);

}

// Drive Pac-Man's mouth cycling in your game loop callback (cbLoop)
void updatePacmanAnimation(Pacman \*pPacman) {
// Only animate if Pac-Man is actively moving
if (pPacman->isMoving) {
pPacman->animTimer++;
if (pPacman->animTimer >= ANIM_DELAY) {
pPacman->animTimer = 0;

            // Cycle animFrame: 0 -> 1 -> 2 -> 3 -> 0
            pPacman->animFrame = (pPacman->animFrame + 1) % FRAMES_PER_DIR;
        }
    } else {
        // Default to a closed mouth frame (frame index 0) when stationary
        pPacman->animFrame = 0;
    }

    // Calculate the 1D index on our tBobSource
    // direction values: RIGHT = 0, LEFT = 1, UP = 2, DOWN = 3
    UWORD uwFrameIndex = (pPacman->direction * FRAMES_PER_DIR) + pPacman->animFrame;

    // Set the frame index on our active ACE Bob instance
    bobSetFrame(g_pPacmanBob, uwFrameIndex);

}
Key Advantages of this ApproachZero Blitter Math: You never have to manually calculate bitplane offsets or pixel coordinates. ACE handles all the offset logic internally.CPU and Blitter Efficiency: bobUpdateAll() performs these operations using optimized Amiga assembly. It avoids calculating screen-buffer offsets multiple times, conserving precious CPU clock cycles on standard Motorola 68000 hardware.Double-Buffering Consistency: Because the Bob references a static tBobSource, you avoid having to synchronize modifications across different double-buffer plane memories.8. Deep Dive: Sheet Layout Optimization (Single Master 320x320 Sheet vs. Split Files)During development, you may wonder: Do I need to split my master 320x320 tilesheet image into a hundred tiny 16x16 files to fit ACE’s frame system?The Short Answer: No!Do not split your files. Keeping your frames together in a single master sheet (often called a tilesheet or texture atlas) is the gold standard for Amiga development. It keeps disk file management clean, ensures your assets load in contiguous blocks of Chip RAM, and makes palette swaps highly efficient.How ACE Solves This (The Asset Pipeline vs. Memory Sharing)You can manage this in your architecture using two primary methods, depending on whether you want your assets compiled sequentially at build time, or if you prefer to slice them dynamically from a shared tBitMap in RAM.Method A: Flattening via the ACE Asset Pipeline (Recommended)You do not need to manually parse grid coordinates on the Amiga CPU during runtime. Instead, you keep editing your single master 320x320 image (e.g., pacman_tiles.png), and configure the ACE Asset Pipeline (specifically the compile-time command line tool bitmap_conv, found in tools/src/bitmap_conv.cpp) to automatically slice, planarize, and index the frames at compile time.How it works:During your make build phase, bitmap_conv reads your 320x320 PNG tilesheet.You configure the compiler flags to slice the input grid into a sequential 1D array of $16 \times 16$ planar frames (an array of bitplane pointers in memory).The final generated output is a clean .BPL binary file containing exactly $400$ sequential frames ($20 \text{ frames horizontally} \times 20 \text{ vertically}$).In your code, bobSourceCreate simply reads the generated .BPL and processes frame 0 as tile (0,0), frame 20 as tile (0,1), and so on, without executing a single division or offset operation on the Amiga's CPU during active gameplay!This completely decouples your artistic workflow (working on a single big canvas in Aseprite or Deluxe Paint) from your Amiga's hardware constraints.Method B: Direct Memory Sharing (Using Multiple BobSources on One Bitmap)If you prefer to load a raw, un-flattened 320x320 bitplane into RAM as a single tBitMap, you can still slice it without copying the graphical memory.In ACE, tBobSource is just a metadata header that points to an underlying tBitMap and stores coordinates. You can instantiate multiple tBobSource definitions that all reference the same master tBitMap while pointing to different offset indices! +-----------------------------+
| Shared Master tBitMap |
| (320 x 320 px) |
+-----------------------------+
/ | \
 / | \
 [Points to Frames 0-15] [Points to 16-31] [Points to 32-40]
/ | \
 v v v
tBobSource *pacman tBobSource *ghosts tBobSource \*fruits
For example, you can load your single master sheet once into memory and segment your metadata slices as follows:#include <ace/managers/bob.h>
#include <ace/utils/bitmap.h>

tBitMap *g_pMasterSheet; // The single 320x320 bitmap in Chip RAM
tBobSource *g_pPacmanSource;
tBobSource \*g_pGhostSource;

void initSharedBobSources(void) {
// 1. Load the single 320x320 sheet once into memory
g_pMasterSheet = bitmapCreateFromFile("bpl/master_sheet.bpl", 0);

    // 2. Create a Pacman BobSource pointing to frames 0-15 in the master sheet
    // Let's assume Pacman occupies the first horizontal strip of the master bitmap
    g_pPacmanSource = bobSourceCreate(g_pMasterSheet, (const UBYTE *)pacman_mask, 16, 16, 16);

    // 3. Create a Ghost BobSource pointing to the next frames in the master sheet
    // By passing an offset pointer or setting up frame-step parameters, you reference the same memory
    g_pGhostSource = bobSourceCreate(g_pMasterSheet, (const UBYTE *)ghost_mask, 16, 16, 16);

    // Note: Both g_pPacmanSource and g_pGhostSource point to the SAME g_pMasterSheet.
    // There is no duplicate bitplane memory, saving valuable Amiga Chip RAM!

}
Architectural Comparison: Blit Coordinates vs. Frame IndexesAspectYour Current CodeMigrated ACE Bob ArchitectureDraw ExecutionManual coordinate calculation: dx = col _ 16, dy = row _ 16. CPU calculates offsets.Direct integer state assign: bobSetFrame(g_pPacmanBob, index).Blit OperationsYour custom drawing function calls blitCopyMask() directly, triggering raw blitter registers.bobUpdateAll() processes all active Bobs automatically in assembly-optimized passes.Double-BufferingCustom frame tracking handles drawing across separate back buffers.Bob background save/restore buffers are automatically maintained across double planes.9. Deep Dive: Implementing Pac-Man Animation Under Method B (Direct Memory Sharing)If you select Method B, keeping your raw un-flattened $320 \times 320$ bitplane in RAM, your animation implementation needs a clear strategy for indexing frames out of a shared graphical grid.Because the master bitmap is $320\text{ pixels}$ wide and your tiles are $16 \times 16\text{ pixels}$, the Amiga's hardware memory represents this sheet as a grid that is exactly 20 tiles wide by 20 tiles high.Master Bitmap (320 x 320)
+---------------------------------------------------+
| Row 0: Pacman frames (0 to 19) | -> Pacman moves here
+---------------------------------------------------+
| Row 1: Ghosts frames (20 to 39) | -> Red/Blue/Pink/Orange Ghosts
+---------------------------------------------------+
| Row 2: Pellet/Fruit frames (40 to 59) |
+---------------------------------------------------+
| ... |
Below, we analyze the two architectural strategies you can use to structure your C callbacks and animation timers to map coordinates directly from this single master bitplane.Strategy A: Global Index Mapping (Recommended)This is the cleanest and most performance-friendly implementation. You instantiate a single master tBobSource representing the entire $320 \times 320$ bitmap. ACE’s inner blitter loops automatically convert any $1\text{D}$ frame index $N$ into $2\text{D}$ coordinates on your sheet using high-performance CPU calculations:$$\text{srcX} = (N \pmod{20}) \times 16$$$$\text{srcY} = (N / 20) \times 16$$Under this pattern, your game logic maps local animation sequences to global frame offsets.1D Grid Addressing Index ReferencePac-Man Moving RIGHT: Global Frames $0 \rightarrow 1 \rightarrow 2 \rightarrow 3$Pac-Man Moving LEFT: Global Frames $4 \rightarrow 5 \rightarrow 6 \rightarrow 7$Ghosts (Red) Moving RIGHT: Global Frames $20 \rightarrow 21$ (Row 1, Columns 0 & 1)Ghosts (Red) Moving LEFT: Global Frames $22 \rightarrow 23$ (Row 1, Columns 2 & 3)C Implementation: Global Frame Offsets#include <ace/managers/bob.h>

#define MASTER_GRID_WIDTH 20 // 20 tiles per row on the 320x320 master sheet
#define TILE_PIXELS 16

tBobSource *g_pMasterSource;
tBob *g_pPacmanBob;
tBob \*g_pRedGhostBob;

void gameInitGraphics(tBitMap *pMasterBitMap, void *pSharedMask, tSimpleBufferManager \*pBuffer) {
// 1. Create one single Source descriptor for the entire sheet
// We specify 400 total frame slots (20x20 grid)
g_pMasterSource = bobSourceCreate(pMasterBitMap, pSharedMask, TILE_PIXELS, TILE_PIXELS, 400);

    // 2. Create your Bob instances pointing to the shared master source
    g_pPacmanBob = bobCreate(g_pMasterSource, TILE_PIXELS, TILE_PIXELS);
    g_pRedGhostBob = bobCreate(g_pMasterSource, TILE_PIXELS, TILE_PIXELS);

    // 3. Push to active draw queue
    bobPush(g_pPacmanBob, pBuffer);
    bobPush(g_pRedGhostBob, pBuffer);

}

// Drive Pac-Man Animation inside your cbLoop step
void animateActors(Pacman *pPacman, Ghost *pGhost) {
// Step A: Animate Pacman (Occupies Row 0, frame offset = 0)
UWORD uwPacmanGlobalFrame = (pPacman->direction \* 4) + pPacman->animFrame;
bobSetFrame(g_pPacmanBob, uwPacmanGlobalFrame);

    // Step B: Animate Red Ghost (Occupies Row 1, frame offset = 20)
    // Local animation frames 0 or 1 map to global frames 20 and 21
    UWORD uwGhostRowOffset = 1 * MASTER_GRID_WIDTH; // Row 1 starts at index 20
    UWORD uwGhostGlobalFrame = uwGhostRowOffset + (pGhost->direction * 2) + pGhost->animFrame;
    bobSetFrame(g_pRedGhostBob, uwGhostGlobalFrame);

}
Strategy B: Virtual BitMap Offsetting (Advanced Memory Slice)If you prefer to separate your codebase cleanly—so that your pacman.c and ghost.c source files use modular, local indices starting at $0$ for their respective tBobSource structs—you can write a custom Amiga routine to slice your master bitmap vertically in Chip RAM.Rather than copying pixels, this technique creates virtual tBitMap headers where the internal bitplane pointer array is shifted downstream into the master buffer.Planar Memory Offsetting MathIn an Amiga planar bitmap, graphic data is arranged as distinct bitplanes. For a standard 5-bitplane, $320\text{-pixel}$ wide screen:Width in Bytes: $320 / 8 = 40\text{ bytes per row}$.Vertical Tile Offset: Each row of tiles is $16\text{ pixels}$ tall.Plane Memory Offset: Offsetting a bitplane pointer down by $R$ tile rows requires shifting the pointer by:$$\text{Offset} = R \times 16 \times 40 \text{ bytes} = R \times 640 \text{ bytes}$$By applying this vertical byte shift to each individual bitplane pointer, we create a lightweight "virtual" bitmap starting exactly at the row we want.+-------------------------------------------------------+
| Master Sheet Bitplane in Chip RAM (320 x 320) |
| [PlanePointer] |
| | |
| v |
| Row 0: Pacman (0-19) |
| | |
| +---- (Offset = 1 _ 16 _ 40 = +640 bytes) |
| | |
| v |
| Row 1: Ghosts (20-39) <--- [VirtualPlanePointer] |
+-------------------------------------------------------+
C Implementation: Virtual Sub-Bitmaps#include <ace/managers/bob.h>
#include <ace/utils/bitmap.h>

tBitMap *g_pMasterSheet;
tBobSource *g_pPacmanSource;
tBobSource \*g_pGhostSource;

// Helper to create a virtual, offset-sliced tBitMap view of memory
tBitMap* createVirtualBitmapSlice(tBitMap *pSource, UWORD uwRowOffset, UWORD uwHeightTiles) {
// 1. Allocate a standard Amiga BitMap structure header (in Fast RAM is fine)
tBitMap _pVirtual = bitmapCreate(pSource->Width, uwHeightTiles _ 16, pSource->Depth, 0);

    // 2. Calculate vertical byte shift (Depth * Width/8 * HeightOffset)
    ULONG ulByteOffset = uwRowOffset * 16 * pSource->BytesPerRow;

    // 3. Shift the virtual pointers to point inside the master's Chip RAM allocation
    for (UBYTE i = 0; i < pSource->Depth; ++i) {
        // Redirect virtual planes directly to master offsets
        pVirtual->Planes[i] = pSource->Planes[i] + ulByteOffset;
    }

    return pVirtual;

}

void initModularSources(void) {
// Load master 320x320 sheet once
g_pMasterSheet = bitmapCreateFromFile("bpl/master_sheet.bpl", 0);

    // Slice Row 0 for Pacman (1 row high, 16 frames max)
    tBitMap *pPacmanVirtualMap = createVirtualBitmapSlice(g_pMasterSheet, 0, 1);
    g_pPacmanSource = bobSourceCreate(pPacmanVirtualMap, (const UBYTE *)pacman_mask, 16, 16, 16);

    // Slice Row 1 for Ghosts (1 row high, 16 frames max)
    tBitMap *pGhostVirtualMap = createVirtualBitmapSlice(g_pMasterSheet, 1, 1);
    g_pGhostSource = bobSourceCreate(pGhostVirtualMap, (const UBYTE *)ghost_mask, 16, 16, 16);

    // NOW, inside ghost.c and pacman.c, local indices work perfectly:
    // bobSetFrame(g_pPacmanBob, localIndex); // References 0-15 locally!

}

void cleanupVirtualSources(void) {
// CRITICAL CRASH PREVENTION:
// Because pVirtual->Planes point inside g_pMasterSheet, you MUST free the virtual
// headers FIRST before destroying the master sheet.

    // 1. Free Bob Sources
    bobSourceDestroy(g_pPacmanSource);
    bobSourceDestroy(g_pGhostSource);

    // 2. Free Master Bitmap (This safely deallocates the real Chip RAM planes)
    bitmapDestroy(g_pMasterSheet);

}
Architectural VerdictUse Strategy A (Global Indexing) if you want the absolute safest implementation. It has zero memory allocations and avoids pointer arithmetic hazards during cleanup.Use Strategy B (Virtual Slicing) if you want clean, modular code where each actor class remains isolated from global grid structures.10. Deep Dive: Hardware Blitter Sprite Layout Alignment Mechanics (ACE vs. Your 48px Padding)In your current hybrid implementation, you use a highly unique sprite asset layout: each $16 \times 16\text{ pixel}$ Pac-Man graphic is saved inside a $48$-pixel wide horizontal block ($16$ empty pixels on the left, $16$ active pixels for Pac-Man, and $16$ empty pixels on the right).While this manually resolved Amiga alignment glitches, ACE manages horizontal shifting in a much more hardware-optimized manner.The Problem: Amiga Blitter Non-Word AlignmentThe Amiga’s custom chipset operates on 16-bit word boundaries in Chip RAM. The hardware blitter can only begin a read or write operation on an address divisible by 2 bytes (16 pixels horizontally in a standard planar layout).When an actor moves across the screen pixel-by-pixel, its horizontal position ($x$) is rarely aligned on a multiple of 16. To render the sprite smoothly, we must handle the fractional remainder:$$\text{shift} = x \pmod{16}$$If $0 < \text{shift} < 16$, the blitter cannot write the sprite directly. It must use its internal hardware barrel shifters (configured via the BLTCON0 and BLTCON1 registers) to shift the bitplane data on the fly as it transfers.Original Word (16px) Shifted by 6 pixels (spills into next word)
[0111111111111110] [0000000111111111][1111100000000000]
|<--- Word 0 --->| |<--- Word 0 --->||<--- Word 1 --->|
Because shifting a 16-pixel wide graphic by $1$ to $15$ pixels causes its data to spill over into the next adjacent word of memory, any shifted blit for a 16-pixel wide Bob must span exactly 2 words ($32$ pixels) wide horizontally.Why Your 48px Padding Method is InefficientTo prevent the shifted bits of Pac-Man from bleeding into adjacent graphics on your sheet, you padded each frame to $48$ pixels (3 words) wide:Your Frame Layout (48 pixels / 3 words):
[ 16px Blank Padding | 16px Active Pac-Man | 16px Blank Padding ]
|<----- Word 0 ----->||<----- Word 1 ----->||<----- Word 2 ----->|
While this successfully prevents graphical corruption during a shifted blit, it introduces a severe hardware bottleneck:DMA Cycle Tax: The blitter must read and write 3 words per line per bitplane.For a 5-bitplane, 16-line Bob, this requires:$$3 \text{ words} \times 16 \text{ lines} \times 5 \text{ bitplanes} \times 4 \text{ channels} = 1,200 \text{ DMA cycles per frame}$$On stock $7.16\text{ MHz}$ Amiga 500 systems, this excessive bus traffic will quickly steal cycles from the Motorola 68000 CPU, causing frame-rate drops.How ACE Optimizes Shift Buffering (The 32px Rule)ACE’s Bob engine employs the standard, high-performance Amiga shift-buffer technique. Rather than wasting memory and DMA cycles on 3-word layouts, ACE requires frames to be exactly 2 words ($32$ pixels) wide:ACE Bob Frame Layout (32 pixels / 2 words):
[ 16px Active Pac-Man | 16px Blank Shift Buffer ]
|<----- Word 0 ----->||<----- Word 1 ----->|
When Pac-Man is drawn at a shifted coordinate (e.g., $x \pmod{16} = 6$):ACE tells the blitter to shift the source graphics by $6\text{ pixels}$ to the right.The $10\text{ pixels}$ that fit in Word 0 stay there, shifted right.The remaining $6\text{ pixels}$ spill over cleanly into Word 1 (the empty shift buffer).Since the right half of the tile is blank, no adjacent graphics are corrupted.The blitter only has to process 2 words (32 pixels) per line instead of 3 words.DMA Savings of the 32px LayoutBy moving from your $48\text{px}$ sheet to ACE’s $32\text{px}$ sheet, you instantly optimize your blits:$$\text{Performance Gain} = \frac{3 \text{ words} - 2 \text{ words}}{3 \text{ words}} \times 100\% = \mathbf{33.3\% \text{ reduction in Blitter load}}$$This frees up massive amounts of Chip RAM bandwidth, allowing you to run your 4 ghosts and Pac-Man simultaneously at a locked $50\text{ Hz}$ on a standard Amiga 500.How to Integrate Your Assets with ACE's 32px SystemWhen migrating to Method B (Direct Memory Sharing), you must adapt your master $320\times320$ sheet slightly to match this architecture.If you keep your raw image format as a $320\text{px}$ wide bitmap, you must ensure that every horizontal sprite tile is structured as a $32\text{px}$ block ($16\text{px}$ sprite on the left, $16\text{px}$ blank space on the right).For example, a master sheet that is $320\text{px}$ wide will contain exactly 10 active Bob frames horizontally (each $32\text{px}$ wide) instead of 20 unpadded tiles:320px Wide BitMap
[ Tile 0: 32px Wide ] [ Tile 1: 32px Wide ] ... [ Tile 9: 32px Wide ]
+-------------------+ +-------------------+ +-------------------+
| 16px Act | 16px B | | 16px Act | 16px B | | 16px Act | 16px B |
+-------------------+ +-------------------+ +-------------------+
Configuring the ACE Source StructWhen instantiating the Bob source under this layout, you specify $16\times16$ as the logical collision/display size, but the underlying bitplane frame-stepping handles the $32\text{px}$ stride:// When initializing with ACE's Bob system:
// - logical width: 16
// - logical height: 16
// - The Bob system automatically detects from the bitmap structure that
// each frame is padded to 32px (2 words) internally to allow shifting.
g_pPacmanSource = bobSourceCreate(g_pMasterSheet, (const UBYTE \*)pacman_mask, 16, 16, 10);
By switching to this system, you retain your single edit-friendly sheet, completely drop your manual $48\text{px}$ alignment calculations, and gain a massive $33\%$ increase in rendering performance!

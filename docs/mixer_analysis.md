# Audio Strategy Analysis: ptplayer vs ace_audio_mixer

## The 4-Channel Hardware Reality

The Amiga has exactly **4 hardware DMA audio channels** (AUD0–AUD3), hardwired in Paula.
Every game ever made on this platform worked within that constraint.

---

## ACE ptplayer (Current Approach)

Located in `framework/ace/src/ace/managers/ptplayer.c`.

- Plays standard **ProTracker `.mod` files** via CIA-B interrupt
- Uses all 4 hardware channels directly
- SFX playback via `ptplayerSfxPlay()` uses **priority-based channel stealing**
- Very low CPU cost — nearly all assembly, interrupt-driven
- Simple API: load mod → load mod → enable music → call `ptplayerProcess()` per frame

---

## ace_audio_mixer (Available in AMIner deps)

Located in `/Users/mattswart/Source/Amiga/AMIner/deps/ace_audio_mixer/`.
Author: Jeroen Knoester, v3.7 (Jan 2025). Written in 68000 assembly.

### What it does differently

The mixer **software-mixes multiple virtual channels into a single output buffer**,
then streams that buffer to one hardware DMA channel. This means you can have many
simultaneous SFX on a single hardware channel, leaving the others free for music.

| | **ACE ptplayer** | **ace_audio_mixer** |
|---|---|---|
| Type | ProTracker MOD player | Software sample mixer |
| Channels | 4 hardware channels directly | Multiple virtual channels → 1–4 hardware |
| Music | ✅ `.mod` natively | ❌ No MOD playback |
| SFX | Up to 4 (one per hardware channel) | Many simultaneous per hardware channel |
| Music + SFX | SFX steal music channels | Designed to co-exist cleanly |
| CPU cost | Very low | Higher (software mixing per frame) |
| Complexity | Low | High (Chip RAM buffers, handler install, config assembly) |

---

## How Classic A500 Games Handled This

### 1. The Standard Split: 2 Music + 2 SFX
The most common approach. Composers deliberately wrote music using only channels 0 and 1,
leaving channels 2 and 3 free for SFX. Music sounds thinner but it works reliably.

### 2. Priority-Based Channel Stealing *(Turrican method)*
Music plays on all 4 channels. When a SFX fires it hijacks the least important music channel
mid-bar. Composers wrote music defensively — melody on channels 0/1, expendable hi-hats
or bass hits on 2/3. `ptplayerSfxPlay()` implements this exact approach.

### 3. SFX Replace Music Entirely
Some games muted all music during action and played raw SFX, then resumed music afterwards.

### 4. Carefully Curated SFX (Design Constraint)
Games simply never triggered more than 1–2 sounds simultaneously by design.
Game rules enforced the hardware limit — can't fire two weapons at once, hits are staggered, etc.

### 5. Paula Volume Tricks
Short percussive sounds faked cheaply by abusing Paula's hardware volume register
without consuming a full DMA channel for a long sample.

### Famous Examples

| Game | Strategy |
|------|----------|
| **Turrican II** | Music on all 4, SFX steal channels — Chris Hülsbeck composed defensively |
| **Lemmings** | 2 channels music, 2 channels SFX |
| **Monkey Island** | Music adapts, SFX interrupt specific voices |
| **Lotus Turbo** | SFX (engine) replaces music during racing |
| **Cannon Fodder** | Strict priority queue — only 1–2 SFX at a time |
| **Superfrog** | Full 4-channel music, SFX steal lowest-priority voice |

---

## Recommendation for Plowman

**Stick with `ptplayer` for now.** The Turrican method (full 4-channel MOD + priority SFX
stealing via `ptplayerSfxPlay()`) is the right approach for a farming/plowing game where
the soundscape is unlikely to be chaotic enough to overwhelm 4 channels.

**Consider `ace_audio_mixer` only if:**
- You need many truly simultaneous SFX (e.g. multiple footsteps, impacts, engine sounds all at once)
- Channel stealing is audibly ruining the music during busy gameplay moments
- You have a composer who can write a music track designed to run on 2–3 channels only

Integration of the mixer is non-trivial: it requires Chip RAM buffer allocation, assembling
`mixer.asm` with a custom `mixer_config.i`, installing its own audio DMA interrupt handler,
and carefully co-ordinating with ACE's system manager.

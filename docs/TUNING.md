# Tuning the face

Everything visual is data. You almost never need to touch the renderer.

## The expression table

`poseFor()` in `src/Face.cpp` is the whole design surface - one `case` per
emotion, each filling in a `Pose`:

| Field | What it does |
| --- | --- |
| `w`, `h` | eye size in pixels |
| `radius` | corner rounding (clamped to half the short side) |
| `spacing` | centre-to-centre distance between the eyes |
| `lidTop`, `lidBot` | fraction of the eye covered from the top / bottom, `0..1` |
| `slant` | pixels; **positive drops the inner corner** (angry), negative drops the outer (sad) |
| `arc` | `0..1`, bites a curve out of the bottom - this is the "smiling eye" |
| `offY` | moves the whole face up/down |
| `tilt` | pixels; left eye down, right eye up. Reads as a head tilt |
| `scaleL`, `scaleR` | per-eye size multiplier. Asymmetry reads as curiosity |
| `gazeX`, `gazeY` | resting gaze, `-1..1` |
| `pupil` | `0..1` pupil size. It sits in whatever band of the eye the lids leave visible and slides with the gaze; `0` for a solid glare (angry) |
| `sparkle` | catchlight in the upper-left of the pupil |
| `bob` | idle breathing amplitude in pixels |
| `style` | `STYLE_EYES`, `STYLE_HEART` or `STYLE_SPIRAL` |

Nothing here is instant: `Face::update()` eases the live pose towards the
target every frame, and expression changes are deliberately hidden behind a
blink (`setEmotion()` triggers one and swaps the pose while the lids are
shut). That single trick is most of why it reads as alive rather than as a
slideshow.

## Adding an emotion

1. Add it to the `Emotion` enum in `src/Face.h`, **before** `EMOTION_COUNT`.
2. Add the same name, in the same order, to `kNames` in `src/Face.cpp`.
3. Add a `case` to `poseFor()`.
4. Optionally give it particles in `Face::applyPose()` (one-shot, when the
   expression starts) or `Face::spawnMoodEffects()` (repeating, while it
   lasts).
5. Optionally add it to `kMoods` in `src/Personality.cpp` so it comes up on
   its own. Those weights add up to 100.

Then look at it without reflashing:

```sh
make preview        # renders preview/emotions.png
```

## Behaviour

`src/Personality.cpp` decides what it does unsupervised: how often the mood
changes, how weighted the mood table is, when boredom sets in, when it falls
asleep, how it reacts to a poke.

The two timings you are most likely to want:

```c
#define DB_IDLE_BORED_S  90.0f   // ignored this long -> visibly bored
#define DB_IDLE_SLEEP_S 300.0f   // ignored this long -> nods off, panel dims
```

## Blink and gaze feel

In `Face::update()`:

- `kClose` / `kOpen` - blink speed. Closing faster than opening is what makes
  a blink look like a blink.
- `nextBlink_` - 2.2-6.5 s normally, with an 18% chance of a quick second
  blink, because real blinks come in clusters.
- The saccade block - how often the eyes wander, and the 34% chance of
  looking back at whoever is sitting there.

## Burn-in

OLEDs do burn in, and a face is the worst case: the same bright shapes in the
same place all day. Two defences are on by default:

- the whole face slowly wanders a few pixels (`DB_BURN_IN_DRIFT`), on two
  periods that never line up;
- the panel dims when the buddy falls asleep.

If you run it 24/7, leave both on.

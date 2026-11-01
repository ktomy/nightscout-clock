# Nightscout mmol/L conversion and delta calculation

How Nightscout converts glucose values and calculates the delta, written so that another implementation can produce the same output.

The behaviour described here is from `cgm-remote-monitor` at commit `c6a0d8ed`. The target output is the delta shown in Nightscout's main web view.

## Principles

- Readings are in mg/dL, and every calculation starts from mg/dL. Conversion to mmol/L happens at the end, for display.
- The conversion factor is **18.01559**, not 18.
- mmol/L values are rounded to 1 decimal place.
- The delta compares averages of 5-minute time buckets, not necessarily two individual readings. With readings every 5 minutes, this is the same as comparing the last two readings.
- The mmol/L delta is not the mg/dL delta converted. Each BG value is converted and rounded first, and then the two are subtracted.

## Rounding

Nightscout runs on JavaScript's `Math.round`, which rounds halves **up, towards positive infinity**:

| Input | `Math.round` (Nightscout) | C/C++ `round()` |
|---|---|---|
| 2.5 | 3 | 3 |
| -2.5 | **-2** | **-3** |

Use this instead of C/C++ `round()`, which rounds halves away from zero:

```
jsRound(x) = floor(x + 0.5)
```

This matters in practice: a gap of 10 minutes with a BG change of 5 mg/dL gives `+3` when rising, but `-2` when falling.

Use 64-bit `double` for all of these calculations. A 32-bit `float` can change the result when a value sits close to a rounding boundary.

## mg/dL to mmol/L

```
mmol = jsRound(mgdl / 18.01559 * 10) / 10
```

BG values are always shown with 1 decimal place, including a trailing zero (`10.0`).

| mg/dL | 40 | 54 | 55 | 70 | 71 | 99 | 100 | 101 | 108 | 109 | 180 | 181 | 250 | 400 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| mmol/L | 2.2 | 3.0 | 3.1 | 3.9 | 3.9 | 5.5 | 5.6 | 5.6 | 6.0 | 6.1 | 10.0 | 10.0 | 13.9 | 22.2 |

Nightscout shows some mg/dL values as text instead of a number:

| mg/dL | Shown as |
|---|---|
| Below 39 | A sensor error code, not a BG value |
| 39 | `LOW` |
| Above 400 | `HIGH` |

## Delta calculation

### 1. Choose the anchor

The anchor is the timestamp of the newest reading that is not in the future. Buckets are measured from this reading, **not from the current time**, so the delta doesn't change or expire as the data ages.

Ignore readings with a timestamp later than the current time.

### 2. Put readings into buckets

There are 4 buckets, each 5 minutes wide. Their position is relative to the anchor:

| Bucket | Window |
|---|---|
| 0 | -2.5 to +2.5 min |
| 1 | -7.5 to -2.5 min |
| 2 | -12.5 to -7.5 min |
| 3 | -17.5 to -12.5 min |

- Both ends of each window are inclusive. A reading exactly on a boundary goes in the **newer** bucket, so a reading at exactly -2.5 min belongs to bucket 0.
- Readings older than -17.5 min are ignored.
- Readings from all devices go in the same buckets, with no deduplication.

### 3. Summarise each bucket

Only readings **above 39 mg/dL** are valid. For the valid readings in each bucket, calculate:

- `mean`: the unrounded average of their mg/dL values
- `time`: the timestamp of the newest one

### 4. Choose two buckets

- `recent` is bucket 0, which always contains the anchor reading.
- `previous` is the first older bucket with at least one valid reading. Empty buckets and buckets with only invalid readings are skipped.

There is **no delta** if bucket 0 has no valid readings or no `previous` bucket exists. Show nothing in that case, not `+0`.

### 5. Calculate

```
absolute    = recent.mean - previous.mean
elapsedMins = (recent.time - previous.time) in minutes, as a fraction
interpolated = elapsedMins > 9

if interpolated:
    mean5MinsAgo = recent.mean - absolute / elapsedMins * 5
else:
    mean5MinsAgo = recent.mean - absolute
```

The elapsed time comes from the newest reading in each bucket, while the difference comes from the bucket averages.

The delta is only scaled to 5 minutes when `elapsedMins` is **over 9**. At 9 minutes or less, the full difference is shown, even though it covers more than 5 minutes.

### 6. Convert to display units

**mg/dL:**
```
delta = jsRound(recent.mean - mean5MinsAgo)
```

**mmol/L:**
```
delta = jsRound((toMmol(recent.mean) - toMmol(mean5MinsAgo)) * 10) / 10
```

`toMmol` is the conversion above, including its rounding. Because each value is rounded before subtracting, the same mg/dL change can give different mmol/L deltas at different BG levels:

| Rise (mg/dL) | mmol/L delta shown, depending on starting BG |
|---|---|
| 1 | `+0` or `+0.1` |
| 2 | `+0.1` or `+0.2` |
| 3 | `+0.1` or `+0.2` |
| 5 | `+0.2` or `+0.3` |
| 9 | `+0.4` or `+0.5` |
| 18 | `+0.9` or `+1` |

### 7. Format

- Add `+` if the delta is zero or positive. Negative values already have `-`.
- Zero is always `+0`, never `-0`. In C/C++, a negative zero can print as `-0`, so check for it.
- **Delta values drop a trailing `.0`**, unlike BG values: `+1` and `-5`, not `+1.0` and `-5.0`. Other values keep their decimal: `+0.2`.
- The web view adds ` *` to the delta when it's interpolated, as in `+3 *`.

## Reference algorithm

```
K = 18.01559
jsRound(x) = floor(x + 0.5)
toMmol(mgdl) = jsRound(mgdl / K * 10) / 10

calculateDelta(readings, now, units):
    past = readings where time <= now
    if past is empty: return none
    anchor = newest time in past

    # for i in 0..3, where the first matching bucket wins
    bucket[i] = past readings where
        anchor - 2.5min - i*5min <= time <= anchor + 2.5min - i*5min

    recent = valid readings (mgdl > 39) in bucket[0]
    if recent is empty: return none

    previous = valid readings in the first of bucket[1..3] that has any
    if none found: return none

    recentMean = mean of recent
    previousMean = mean of previous
    absolute = recentMean - previousMean
    elapsedMins = (newest time in recent - newest time in previous) / 60000

    if elapsedMins > 9:
        mean5MinsAgo = recentMean - absolute / elapsedMins * 5
    else:
        mean5MinsAgo = recentMean - absolute

    if units == mmol:
        return jsRound((toMmol(recentMean) - toMmol(mean5MinsAgo)) * 10) / 10
    return jsRound(recentMean - mean5MinsAgo)
```

This algorithm was checked against Nightscout's own code using randomly generated data: 60,000 sets of readings in both units, for 120,000 comparisons and no differences. The data included irregular timing, gaps, invalid readings, several readings per bucket, and future readings.

## JavaScript vs C++ hints

Nightscout's behaviour comes partly from how JavaScript handles numbers. These are the places where a direct C++ translation can give a different result:

| Topic | JavaScript (Nightscout) | C++ | What to do |
|---|---|---|---|
| Rounding halves | `Math.round(-2.5)` is `-2` | `std::round(-2.5)` is `-3` | Use `floor(x + 0.5)` |
| Number type | Every number is a 64-bit double | `float` is 32-bit, and `f` literals such as `18.01559f` are too | Use `double` and `18.01559` |
| Division | `(100 + 101) / 2` is `100.5` | With `int`, it's `100` | Use `double` for sums and counts |
| Elapsed time | `570000 / 60000` is `9.5` | With integers, it's `9`, which misses the over-9-minute check | Divide by `60000.0` |
| Timestamps | Milliseconds fit in a double | Overflows `int32_t` and `uint32_t` | Use `int64_t` |
| Time differences | Can go negative | Wraps to a huge number with unsigned types | Use signed types, or compare before subtracting |
| Negative zero | `-0 >= 0` is true, and it prints as `0` | `-0 >= 0` is true, but it prints as `-0` | Format the absolute value and add the sign yourself |
| Trailing `.0` | The number `1.0` prints as `1` | `%.1f` prints `1.0` | Remove `.0` from delta values, not BG values |

Other points:

- **Arduino's `round()`:** it also rounds halves away from zero, so it has the same problem as `std::round`.
- **Don't let `printf` do the rounding:** `%.1f` works from the exact binary value, so `printf("%.1f", 5.55)` gives `5.5`, while `floor(5.55 * 10 + 0.5) / 10` gives `5.6`. Round first, then format the result.
- **`float` may look fine at first:** for whole mg/dL values from 40 to 400, `float` and `double` gave the same mmol/L text. Bucket averages and interpolated values are fractional, so they aren't covered by that check.
- **ESP32 `double`:** it's a true 64-bit type, calculated in software. That's slower than `float`, but this calculation runs rarely enough for it not to matter.
- **Order of operations:** keep the formulas as written, such as `recentMean - (recentMean - absolute)`, instead of simplifying them. Floating-point results can differ if the steps change.

These helpers give the expected results:

```cpp
#include <cmath>
#include <cstdio>
#include <cstring>

double jsRound(double x) {
  return std::floor(x + 0.5);
}

double toMmol(double mgdl) {
  return jsRound(mgdl / 18.01559 * 10) / 10;
}

void formatDelta(char *out, size_t size, double delta, bool mmol) {
  char value[16];
  snprintf(value, sizeof value, mmol ? "%.1f" : "%.0f", std::fabs(delta));

  size_t length = strlen(value);
  if (length > 2 && strcmp(value + length - 2, ".0") == 0) {
    value[length - 2] = '\0';
  }

  snprintf(out, size, "%c%s", delta < 0 ? '-' : '+', value);
}
```

## Test cases

These results come from running Nightscout's own code. Times are in minutes relative to the current time, and each reading is `time: mg/dL`.

| Scenario | Readings | mg/dL | mmol/L | Interpolated |
|---|---|---|---|---|
| Rising | -5: 100, 0: 105 | `+5` | `+0.2` | No |
| Falling | -5: 105, 0: 100 | `-5` | `-0.2` | No |
| Flat | -5: 120, 0: 120 | `+0` | `+0` | No |
| Rounds to the same mmol/L | -5: 100, 0: 101 | `+1` | `+0` | No |
| Crosses an mmol/L step | -5: 108, 0: 109 | `+1` | `+0.1` | No |
| Large drop | -5: 200, 0: 110 | `-90` | `-5` | No |
| 8 min gap, not scaled | -8: 100, 0: 108 | `+8` | `+0.4` | No |
| Exactly 9 min, not scaled | -9: 100, 0: 109 | `+9` | `+0.5` | No |
| 9.5 min gap, scaled | -9.5: 100, 0: 110 | `+5` | `+0.3` | Yes |
| 10 min gap, rising | -10: 100, 0: 105 | `+3` | `+0.1` | Yes |
| 10 min gap, falling (half rounds up) | -10: 105, 0: 100 | `-2` | `-0.1` | Yes |
| Small fall rounds to zero | -10: 121, 0: 120 | `+0` | `+0` | Yes |
| 15 min gap | -15: 100, 0: 115 | `+5` | `+0.3` | Yes |
| Previous reading too old | -20: 100, 0: 120 | None | None | - |
| Only one reading | 0: 100 | None | None | - |
| Readings every minute | -7: 93, -6: 94, -5: 95, -4: 96, -3: 97, -2: 98, -1: 99, 0: 100 | `+4` | `+0.2` | No |
| Two readings in the recent bucket | -5: 100, -2: 110, 0: 120 | `+15` | `+0.8` | No |
| Reading exactly on a bucket boundary | -5: 100, -2.5: 110, 0: 130 | `+20` | Not run | No |
| Invalid reading in the previous bucket | -10: 100, -5: 30, 0: 110 | `+5` | `+0.3` | Yes |
| 39 in the previous bucket | -10: 100, -5: 39, 0: 110 | `+5` | `+0.3` | Yes |
| Newest reading is invalid | -5: 100, 0: 30 | None | None | - |
| Reading in the future | -5: 100, 0: 105, +5: 200 | `+5` | `+0.2` | No |

## Behaviour to replicate, not fix

These can look like bugs, but changing them would make the output differ from Nightscout:

- **Sensors reading every minute read low.** The bucket averages are centred about 4 minutes apart, so a steady rise of 1 mg/dL per minute shows as `+4`, not `+5`. The current BG shown is the last reading, but the delta uses the bucket average.
- **Gaps of 5 to 9 minutes aren't scaled.** An 8-minute change is shown in full.
- **The delta doesn't expire.** It's calculated from the last reading, however old that is.

## Other places Nightscout shows a delta

These can legitimately differ from the main view:

- **Clock pages:** when a clock's units differ from the site's, it converts the rounded mg/dL delta directly and keeps the decimal. A 3 mg/dL rise is always `+0.2`, and 18 mg/dL is `+1.0`.
- **Pebble endpoint:** in mmol/L, it always formats the delta to 1 decimal place, such as `1.0`, and has no `+` sign.

## Checking against a live site

`/api/v2/properties/delta` returns a site's current delta, with `display`, `mgdl`, `scaled`, `interpolated`, `elapsedMins`, `absolute` and `mean5MinsAgo`. It uses the site's configured units.

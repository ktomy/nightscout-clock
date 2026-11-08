#ifndef GlucoseUnits_h
#define GlucoseUnits_h

// This header stays free of Arduino dependencies so that test/ can compile it on the host.

// mg/dL per mmol/L, the factor Nightscout uses. See docs/NIGHTSCOUT_MMOL_AND_DELTA.md. The web UI keeps
// its own copy in web/src/js/schema.js.
constexpr double MGDL_PER_MMOL = 18.01559;

#endif

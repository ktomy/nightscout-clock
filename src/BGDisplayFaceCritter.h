#ifndef BGDISPLAYFACECRITTER_H
#define BGDISPLAYFACECRITTER_H

#include "BGDisplayFaceTextBase.h"
#include "BGDisplayFaceWithAge.h"

// Cute character faces. Each critter is a 13x8 sprite; the body color
// follows the glucose level (natural/yellow/red) or goes gray when stale.
// Palette: 0=transparent, 1=body (dynamic), 2=accent, 3=eye, 4-7=extra static colors.
enum class CritterId : uint8_t {
    CAT = 0,
    DOG,
    FROG,
    FOX,
    BUNNY,
    // Animals
    NARWHAL,
    WHALE,
    // Mario
    MARIO,
    LUIGI,
    PEACH,
    TOAD,
    // Halloween / Fall
    PUMPKIN,
    GHOST,
    WITCH,
    TURKEY,
    // Other cute
    BUTTERFLY,
    COUNT
};

class BGDisplayFaceCritter : public BGDisplayFaceTextBase, public BGDisplayFaceWithAge {
public:
    explicit BGDisplayFaceCritter(CritterId id);
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;

private:
    CritterId critterId;
    const uint8_t* getSprite() const;
    const uint16_t* getPalette(BG_LEVEL level, bool dataIsOld) const;
    void drawSprite(const uint8_t* sprite, const uint16_t* palette) const;
};

#endif  // BGDISPLAYFACECRITTER_H

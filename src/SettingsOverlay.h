#ifndef SETTINGSOVERLAY_H
#define SETTINGSOVERLAY_H

#include <ArduinoJson.h>

// Lays `over` onto `base`. Objects merge key by key at every depth; anything else in `over`, a value
// or an array, replaces what `base` has. Stored settings laid over the factory settings this way keep
// every saved value, and a setting added in a later firmware takes its factory value, however deeply
// it is nested.
inline void overlaySettings(JsonVariant base, JsonVariantConst over) {
    if (!over.is<JsonObjectConst>() || !base.is<JsonObject>()) {
        base.set(over);
        return;
    }
    JsonObject target = base.as<JsonObject>();
    for (JsonPairConst pair : over.as<JsonObjectConst>()) {
        JsonVariant existing = target[pair.key()];
        if (pair.value().is<JsonObjectConst>() && existing.is<JsonObject>()) {
            overlaySettings(existing, pair.value());
        } else {
            target[pair.key()] = pair.value();
        }
    }
}

#endif

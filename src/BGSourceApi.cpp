#include <Arduino.h>

#include "BGSourceApi.h"
#include "ServerManager.h"

void BGSourceApi::setup() {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("BGSourceApi::setup");
#endif

    // To be able to add handlers we need to remove the default handler, this is kinda dirty hack, but there is no other way
    // (known to me)
    ServerManager.removeStaticFileHandler();

    ArJsonRequestHandlerFunction entriesPostHandler = [this](AsyncWebServerRequest *request, JsonVariant &json) {
        this->HandleEntriesPost(request, json);
    };

    ServerManager.addHandler(new AsyncCallbackJsonWebHandler("/api/v1/entries", entriesPostHandler));

    ServerManager.addHandler(createDeleteEntriesHandler());

    // After adding our handlers we need to add the default handler back
    ServerManager.addStaticFileHandler();
}

AsyncCallbackWebHandler *BGSourceApi::createDeleteEntriesHandler() {
    auto handler = new AsyncCallbackWebHandler();
    handler->setUri("/api/v1/entries");
    handler->setMethod(HTTP_DELETE);
    handler->onRequest([this](AsyncWebServerRequest *request) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("BGSourceApi::HandleEntriesDelete");
#endif
        glucoseReadings.clear();
        hasNewDataFlag = true;
        request->send(200, "application/json", "{\"status\": \"ok\"}");
    });
    return handler;
}

void BGSourceApi::HandleEntriesPost(AsyncWebServerRequest *request, JsonVariant &json) {

#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("BGSourceApi::HandleEntriesPost");
#endif

    if (not json.is<JsonArray>()) {
        request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
        return;
    }
    auto &&data = json.as<JsonArray>();
    for (auto &&entry : data) {
        if (not entry.is<JsonObject>()) {
            request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
            return;
        }
        auto &&entryObj = entry.as<JsonObject>();
        if (not entryObj.containsKey("dateString") or not entryObj.containsKey("sgv")) {
            request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
            return;
        }
        auto sgv = entryObj["sgv"].as<int>();
        auto direction = entryObj["direction"].as<String>();
        auto trend = entryObj["trend"].as<int>();
        auto date = entryObj["date"].as<unsigned long long>();
        auto glucoseReading = GlucoseReading();
        glucoseReading.sgv = sgv;
        glucoseReading.trend = parseDirection(direction);
        if (glucoseReading.trend == BG_TREND::NONE) {
            glucoseReading.trend = static_cast<BG_TREND>(trend);
        }

        glucoseReading.epoch = date / 1000;
        glucoseReadings.push_back(glucoseReading);
    }

    hasNewDataFlag = true;

    request->send(200, "application/json", "{\"status\": \"ok\"}");
}

std::list<GlucoseReading> BGSourceApi::updateReadings(std::list<GlucoseReading> existingReadings) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("BGSourceApi::updateReadings");
#endif
    firstConnectionSuccess = true;
    return existingReadings;
}

bool BGSourceApi::hasNewData(unsigned long long epoch) {
    if (hasNewDataFlag) {
        hasNewDataFlag = false;
        return true;
    }
    return false;
}

#include <Arduino.h>

#include "BGSourceApi.h"
#include "ServerManager.h"

void BGSourceApi::setup() {
    DEBUG_PRINTLN("BGSourceApi::setup");

    ArJsonRequestHandlerFunction entriesPostHandler = [this](AsyncWebServerRequest *request,
                                                             ArduinoJson::V6214PB2::JsonVariant &json) {
        this->HandleEntriesPost(request, json);
    };

    ServerManager.addHandler(new AsyncCallbackJsonWebHandler("/api/v1/entries", entriesPostHandler));

    ServerManager.addHandler(new AsyncCallbackJsonWebHandler(
        "/api/v1/entries", [this](AsyncWebServerRequest *request, ArduinoJson::V6214PB2::JsonVariant &json) {
            glucoseReadings.clear();
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        }));

    firstConnectionSuccess = true;
}

void BGSourceApi::HandleEntriesPost(AsyncWebServerRequest *request, JsonVariant &json) {

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
    request->send(200, "application/json", "{\"status\": \"ok\"}");
}

std::list<GlucoseReading> BGSourceApi::updateReadings(std::list<GlucoseReading> existingReadings) { return existingReadings; }

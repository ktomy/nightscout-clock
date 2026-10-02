#include "BGDisplayFaceTitleScroll.h"
#include "BGDisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Hash.h>
#include <WiFi.h>

namespace {
String cachedCustomTitle = "Nightscout";
bool isFetchingTitle = false;
int16_t tScrollX = 32;
unsigned long tLastStep = 0, tPauseStart = 0;
bool tIsPaused = false;

void fetchTitleTask(void* param) {
    String baseUrl = SettingsManager.settings.nightscout_url;
    String token = SettingsManager.settings.nightscout_api_key;
    // No Nightscout endpoint configured: keep the default title, nothing to fetch.
    if (baseUrl.length() == 0 || token.length() == 0) {
        isFetchingTitle = false;
        vTaskDelete(NULL);
    }
    if (baseUrl.endsWith("/")) baseUrl.remove(baseUrl.length() - 1);
    if (!baseUrl.startsWith("http")) baseUrl = "https://" + baseUrl;

    String url = baseUrl + "/api/v1/status.json?token=" + token;
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(4000);

    HTTPClient http;
    if (http.begin(client, url)) {
        http.setTimeout(4000);
        http.addHeader("Accept", "application/json");
        if (http.GET() == 200) {
            String payload = http.getString();
            JsonDocument doc, filter;
            filter["settings"]["customTitle"] = true;
            filter["settings"]["title"] = true;
            filter["customTitle"] = true;
            filter["title"] = true;

            if (!deserializeJson(doc, payload, DeserializationOption::Filter(filter))) {
                String found = "";
                if (doc["settings"]["customTitle"].is<String>()) found = doc["settings"]["customTitle"].as<String>();
                else if (doc["settings"]["title"].is<String>()) found = doc["settings"]["title"].as<String>();
                else if (doc["customTitle"].is<String>()) found = doc["customTitle"].as<String>();
                if (found.length() > 0) cachedCustomTitle = found;
            }
        }
        http.end();
    }
    isFetchingTitle = false;
    vTaskDelete(NULL);
}
}

void BGDisplayFaceTitleScroll::onActivate() const {
    tScrollX = 32; tLastStep = 0; tPauseStart = 0; tIsPaused = false;
    if (!isFetchingTitle && WiFi.isConnected()) {
        isFetchingTitle = true;
        if (xTaskCreatePinnedToCore(fetchTitleTask, "titleFetch", 8192, NULL, 1, NULL, 1) != pdPASS) {
            isFetchingTitle = false;
        }
    }
}
bool BGDisplayFaceTitleScroll::needsFrequentRefresh() const { return true; }
unsigned long BGDisplayFaceTitleScroll::getFrequentRefreshIntervalMs() const { return 20; }

void BGDisplayFaceTitleScroll::showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld) const { showTitleTrain(readings, dataIsOld); }
void BGDisplayFaceTitleScroll::showNoData() const { std::list<GlucoseReading> empty; showTitleTrain(empty, true); }

void BGDisplayFaceTitleScroll::showTitleTrain(const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    unsigned long now = millis();
    String bgStr = "---", deltaStr = "";
    bool towards = true;
    GlucoseReading lastReading{0, BG_TREND::NONE, 0};

    if (!readings.empty()) {
        lastReading = readings.back();
        bgStr = getPrintableReading(lastReading.sgv);
        if (readings.size() >= 2) {
            auto it = readings.rbegin(); it++;
            int d = lastReading.sgv - it->sgv;
            deltaStr = (d >= 0 ? "+" : "") + getPrintableReading(d);
            // sgv is stored in mg/dL internally regardless of display units.
            towards = (lastReading.sgv > 180 && d < 0) || (lastReading.sgv < 70 && d > 0) || (lastReading.sgv >= 70 && lastReading.sgv <= 180);
        }
    }

    // Measure with the font used for drawing below; getTextWidth uses currentFont.
    DisplayManager.setFont(FONT_TYPE::MEDIUM);
    int titleW = DisplayManager.getTextWidth(cachedCustomTitle.c_str(), 2);
    int bgW = DisplayManager.getTextWidth(bgStr.c_str(), 2);
    int deltaW = deltaStr.length() > 0 ? DisplayManager.getTextWidth(deltaStr.c_str(), 2) : 0;
    int statsW = bgW + 2 + 5 + (deltaW > 0 ? (3 + deltaW) : 0);
    int centerTarget = max(0, (32 - statsW) / 2);

    if (tIsPaused) {
        if (now - tPauseStart > 3500) tIsPaused = false;
    } else if (now - tLastStep > 20) {
        tScrollX--;
        if (tScrollX + titleW + 10 <= centerTarget) {
            tIsPaused = true;
            tPauseStart = now;
        }
        if (tScrollX < -(titleW + 10 + statsW)) tScrollX = 32;
        tLastStep = now;
    }

    int curStats = tScrollX + titleW + 10;
    int curBg = curStats, curArr = curBg + bgW + 2, curDelta = curArr + 5 + 3;

    DisplayManager.setTextColor(COLOR_CYAN);
    DisplayManager.printText(tScrollX, 6, cachedCustomTitle.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);

    uint16_t bgCol = dataIsOld ? getDataOldColor() : getDisplayColorByBGValue(lastReading);
    DisplayManager.setTextColor(bgCol);
    DisplayManager.printText(curBg, 6, bgStr.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);

    if (!readings.empty()) showTrendArrow(lastReading, curArr, 1, dataIsOld, false, false);
    if (deltaStr.length() > 0) {
        uint16_t dCol = dataIsOld ? getDataOldColor() : (towards ? (uint16_t)COLOR_GREEN : (uint16_t)COLOR_YELLOW);
        DisplayManager.setTextColor(dCol);
        DisplayManager.printText(curDelta, 6, deltaStr.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);
    }
    if (!readings.empty()) drawTimerBlocks(lastReading, MATRIX_WIDTH, 0, 7);
}

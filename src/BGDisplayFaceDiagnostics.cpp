#include "BGDisplayFaceDiagnostics.h"

#include "ServerManager.h"
#include "globals.h"

namespace {
constexpr int CONTENT_BASELINE_Y = 7;
constexpr int STATUS_ITEM_SPACING = 2;
constexpr int ARROW_SPACING = 1;
constexpr int ARROW_WIDTH = 5;
constexpr int DATETIME_SCROLL_LEFT_PADDING = 2;
// ~30fps. Each frame drives a full WS2812 blit (~7-8ms, interrupts disabled),
// so 60fps left little headroom for WiFi/buttons; 30fps still scrolls smoothly.
// The step is scaled with the frame time to keep the on-screen scroll speed the
// same (speed = step / frame_ms).
constexpr unsigned long DATETIME_SCROLL_FRAME_MS = 33;
constexpr float DATETIME_SCROLL_STEP_PIXELS = 0.37f;
const char* WEEKDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
}  // namespace

void BGDisplayFaceDiagnostics::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showDateTimePage(readings, dataIsOld);
}

void BGDisplayFaceDiagnostics::showNoData() const {
    const String& dateTimeText = formatDateTime();
    String noDataText = SettingsManager.settings.bg_units == BG_UNIT::MMOLL ? "--.-" : "---";
    int dateTimeWidth = (int)DisplayManager.getTextWidth(dateTimeText.c_str(), 2);
    int noDataWidth = (int)DisplayManager.getTextWidth(noDataText.c_str(), 2);
    int contentWidth = dateTimeWidth + STATUS_ITEM_SPACING + noDataWidth;
    int scrollX = getScrollX(contentWidth);

    DisplayManager.clearMatrix(false);

    DisplayManager.setTextColor(COLOR_CYAN);
    DisplayManager.printText(scrollX, CONTENT_BASELINE_Y, dateTimeText.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);

    DisplayManager.setTextColor(BG_COLOR_OLD);
    DisplayManager.printText(
        scrollX + dateTimeWidth + STATUS_ITEM_SPACING, CONTENT_BASELINE_Y, noDataText.c_str(),
        TEXT_ALIGNMENT::LEFT, 2, false);

    DisplayManager.update();
}

bool BGDisplayFaceDiagnostics::needsFrequentRefresh() const { return contentScrolls; }

unsigned long BGDisplayFaceDiagnostics::getFrequentRefreshIntervalMs() const {
    return DATETIME_SCROLL_FRAME_MS;
}

void BGDisplayFaceDiagnostics::showDateTimePage(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    auto lastReading = readings.back();
    const String& dateTimeText = formatDateTime();
    String printableReading = getPrintableReading(lastReading.sgv);
    int dateTimeWidth = (int)DisplayManager.getTextWidth(dateTimeText.c_str(), 2);
    int readingWidth = (int)DisplayManager.getTextWidth(printableReading.c_str(), 2);
    int readingX = dateTimeWidth + STATUS_ITEM_SPACING;
    int arrowX = readingX + readingWidth + ARROW_SPACING;
    int contentWidth = arrowX + ARROW_WIDTH;
    int scrollX = getScrollX(contentWidth);

    DisplayManager.clearMatrix(false);

    DisplayManager.setTextColor(COLOR_CYAN);
    DisplayManager.printText(scrollX, CONTENT_BASELINE_Y, dateTimeText.c_str(), TEXT_ALIGNMENT::LEFT, 2, false);

    showReading(
        lastReading, scrollX + readingX, CONTENT_BASELINE_Y, TEXT_ALIGNMENT::LEFT, FONT_TYPE::MEDIUM,
        dataIsOld, false);
    showTrendArrow(lastReading, scrollX + arrowX, 2, dataIsOld, true, false);

    DisplayManager.update();
}

int BGDisplayFaceDiagnostics::getScrollX(int contentWidth) const {
    // When everything fits we render a static, centered frame; there is nothing
    // to animate, so drop back to the normal per-minute refresh (see
    // needsFrequentRefresh) instead of redrawing at the scroll frame rate.
    contentScrolls = contentWidth > MATRIX_WIDTH;
    if (!contentScrolls) {
        return max(0, (MATRIX_WIDTH - contentWidth) / 2);
    }

    float travel = contentWidth + MATRIX_WIDTH + DATETIME_SCROLL_LEFT_PADDING;
    float position = MATRIX_WIDTH - fmodf(
                                        (millis() / (float)DATETIME_SCROLL_FRAME_MS) *
                                            DATETIME_SCROLL_STEP_PIXELS,
                                        travel);

    return (int)position;
}

const String& BGDisplayFaceDiagnostics::formatDateTime() const {
    unsigned long now = millis();

    // getTimezonedTime() wraps getLocalTime(), which is comparatively expensive
    // and can briefly block, so re-read the clock at most about once per second
    // even though this runs at the scroll frame rate.
    if (cachedDateTime.length() == 0 || now - lastTimeCheckMillis >= 1000) {
        lastTimeCheckMillis = now;
        tm timeinfo = ServerManager.getTimezonedTime();

        // The rendered text only shows down to the minute, so only rebuild the
        // String when the minute actually changes.
        int minuteKey =
            ((timeinfo.tm_mon * 32 + timeinfo.tm_mday) * 24 + timeinfo.tm_hour) * 60 + timeinfo.tm_min;
        if (minuteKey != cachedMinuteKey) {
            cachedMinuteKey = minuteKey;
            char dateTimeBuffer[20];
            snprintf(
                dateTimeBuffer, sizeof(dateTimeBuffer), "%s %02d/%02d %02d:%02d",
                WEEKDAY_NAMES[timeinfo.tm_wday], timeinfo.tm_mday, timeinfo.tm_mon + 1,
                timeinfo.tm_hour, timeinfo.tm_min);
            cachedDateTime = String(dateTimeBuffer);
        }
    }

    return cachedDateTime;
}

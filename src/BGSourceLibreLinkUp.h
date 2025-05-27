#ifndef BGSOURCELIBRELINKUP_H
#define BGSOURCELIBRELINKUP_H

#include <mbedtls/sha256.h>

#include <map>

#include "BGSource.h"
#include "SettingsManager.h"

#define LIBRE_LINK_UP_VERSION "4.12.0"
#define LIBRE_LINK_UP_PRODUCT "llu.ios"
#define USER_AGENT                                                                              \
    "Mozilla/5.0 (iPhone; CPU OS 17_4.1 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) " \
    "Version/17.4.1 "                                                                           \
    "Mobile/10A5355d "                                                                          \
    "Safari/8536.25"

class AuthTicket {
public:
    AuthTicket() = default;
    AuthTicket(String token, unsigned long long expires, unsigned long long duration, String accountId)
        : token(token), expires(expires), duration(duration), accountId(accountId) {}
    unsigned long long expires;
    unsigned long long duration;
    String token;
    String accountId;
    String patientId;
};

class BGSourceLibreLinkUp : public BGSource {
public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;

private:
    bool hasValidAuthentication();
    void deleteAuthTicket();
    AuthTicket login();
    AuthTicket authTicket;
    std::list<GlucoseReading> getReadings(unsigned long long lastReadingEpoch);
    GlucoseReading getLibreLinkUpConnection();
    String encodeSHA256(String toEncode);
    unsigned long long libreTimestampToEpoch(String dateString);
    std::map<String, String> standardHeaders = {
        {"User-Agent", USER_AGENT},
        // {"Accept", "application/json"},
        {"Content-Type", "application/json;charset=UTF-8"},
        {"version", LIBRE_LINK_UP_VERSION},
        {"product", LIBRE_LINK_UP_PRODUCT}};
    std::map<String, String> libreEndpoints = {
        {"AE", "api-ae.libreview.io"},   {"AP", "api-ap.libreview.io"}, {"AU", "api-au.libreview.io"},
        {"CA", "api-ca.libreview.io"},   {"DE", "api-de.libreview.io"}, {"EU", "api-eu.libreview.io"},
        {"EU2", "api-eu2.libreview.io"}, {"FR", "api-fr.libreview.io"}, {"JP", "api-jp.libreview.io"},
        {"US", "api-us.libreview.io"},   {"LA", "api-la.libreview.io"}, {"RU", "api.libreview.ru"}};
};

#endif  // BGSOURCELIBRELINKUP_H
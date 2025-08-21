#ifndef BGSOURCEMEDTRONIC_H
#define BGSOURCEMEDTRONIC_H

#include "BGSource.h"
#include <ArduinoJson.h>

// Structure to hold Medtronic authentication tokens
struct MedtronicTokenData {
    String access_token;
    String refresh_token;
    String client_id;
    String client_secret;
    String mag_identifier;
    String scope;
    unsigned long long expires_at;
    
    bool isValid() const {
        return !access_token.isEmpty() && !client_id.isEmpty() && 
               !client_secret.isEmpty() && !mag_identifier.isEmpty();
    }
    
    bool isExpired() const {
        if (expires_at == 0) return false;
        // expires_at is in milliseconds since epoch, convert to seconds and compare with current time
        unsigned long long currentTimeSeconds = time(NULL);
        unsigned long long expiresAtSeconds = expires_at / 1000;
        return currentTimeSeconds > expiresAtSeconds;
    }
};

class BGSourceMedtronic : public BGSource {
public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;

private:
    // Token management
    bool loadTokenData(MedtronicTokenData& tokenData);
    bool saveTokenData(const MedtronicTokenData& tokenData);
    bool refreshAccessToken(MedtronicTokenData& tokenData);
    
    // Data fetching
    bool fetchRecentData(const MedtronicTokenData& tokenData, String& jsonData);
    std::list<GlucoseReading> parseGlucoseReadings(const String& jsonData);
    
    // Helper functions
    String getDiscoveryUrl(const String& country);
    String makeAuthenticatedRequest(const String& url, const MedtronicTokenData& tokenData);
    BG_TREND parseTrendArrow(const String& trend);
    
    // Constants
    static const unsigned long TOKEN_REFRESH_MARGIN_MS;
    static const unsigned long REQUEST_TIMEOUT_MS;
};

#endif  // BGSOURCEMEDTRONIC_H
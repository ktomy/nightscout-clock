#ifndef BGSOURCEMEDTRONIC_H
#define BGSOURCEMEDTRONIC_H

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LCBUrl.h>
#include <WiFiClientSecure.h>

#include <optional>

#include "BGSource.h"
#include "utils/CachedData.h"
#include "utils/JWTUtils.h"

// Token refresh margin in seconds (15 minutes)
static const unsigned long TOKEN_REFRESH_MARGIN_SECONDS = 15 * 60;

// Structure to hold Medtronic authentication tokens
struct MedtronicTokenData {
private:
    String access_token;
    String refresh_token;
    String client_id;
    String client_secret;
    String mag_identifier;
    String scope;
    unsigned long long expires_at = 0;

    // Extracted fields from access token payload for convenience
    String access_token_preferred_username;
    String access_token_country;

public:
    // Constructor for initial token data (without expires_at)
    MedtronicTokenData(
        const String& clientId, const String& clientSecret, const String& magIdentifier,
        const String& scopeValue)
        : client_id(clientId),
          client_secret(clientSecret),
          mag_identifier(magIdentifier),
          scope(scopeValue) {}

    // Default constructor
    MedtronicTokenData() = default;

    // Getters
    const String& getAccessToken() const { return access_token; }
    const String& getRefreshToken() const { return refresh_token; }
    const String& getClientId() const { return client_id; }
    const String& getClientSecret() const { return client_secret; }
    const String& getMagIdentifier() const { return mag_identifier; }
    const String& getScope() const { return scope; }
    unsigned long long getExpiresAt() const { return expires_at; }
    const String& getAccessTokenCountry() const { return access_token_country; }
    const String& getAccessTokenPreferredUsername() const { return access_token_preferred_username; }

    bool isExpired() const {
        if (expires_at == 0)
            return false;

        // Consider token expired 15 minutes before actual expiration for safety
        return ServerManager.getUtcEpoch() > (expires_at - TOKEN_REFRESH_MARGIN_SECONDS);
    }

    // Set access token and refresh token, extract convenience fields from JWT payload
    bool setAccessToken(const String& accessToken, const String& refreshToken = "") {
        // First validate the access token by extracting its payload
        std::optional<JsonDocument> optionalAccessTokenPayload = JWTUtils::getJWTPayload(accessToken);
        if (!optionalAccessTokenPayload) {
            return false;
        }

        JsonDocument accessTokenPayload = optionalAccessTokenPayload.value();

        // Validate that token_details exists
        if (!accessTokenPayload["token_details"].is<JsonObject>()) {
            return false;
        }

        JsonObject tokenDetails = accessTokenPayload["token_details"];
        String country = tokenDetails["country"].as<String>();
        String username = tokenDetails["preferred_username"].as<String>();

        // Only proceed if both required fields are available
        if (country.isEmpty() || username.isEmpty()) {
            return false;
        }

        // All validations passed - now set the values
        access_token = accessToken;
        if (!refreshToken.isEmpty()) {
            refresh_token = refreshToken;
        }
        access_token_country = country;
        access_token_preferred_username = username;

        // Set expiration from JWT exp claim (standard approach)
        expires_at = accessTokenPayload["exp"].as<unsigned long>();

        return true;
    }  // Serialize token data to JSON string
    String toJson() const {
        JsonDocument doc;
        doc["access_token"] = access_token;
        doc["refresh_token"] = refresh_token;
        doc["client_id"] = client_id;
        doc["client_secret"] = client_secret;
        doc["mag-identifier"] = mag_identifier;
        doc["scope"] = scope;

        String jsonString;
        serializeJson(doc, jsonString);
        return jsonString;
    }
};

// Structure to hold Medtronic CareLink API configuration
struct MedtronicConfig {
    String region;
    String base_url_carelink;
    String base_url_cumulus;
    String base_url_pde;
    String base_url_aem;
    String carelink_server_url;  // Pre-computed CareLink server URL based on region
    String token_url;            // Token endpoint URL from SSO configuration
};

// Structure to hold Medtronic user and patient information
struct MedtronicUserInfo {
    String role;                            // User role (extracted for convenience)
    String firstName;                       // User's first name
    String lastName;                        // User's last name
    std::optional<String> patientUsername;  // Username of the patient (for care partners)

    // Default constructor
    MedtronicUserInfo() = default;

    // Constructor for patient users
    MedtronicUserInfo(const String& userRole, const String& userFirstName, const String& userLastName)
        : role(userRole), firstName(userFirstName), lastName(userLastName) {}

    // Constructor for care partner users
    MedtronicUserInfo(
        const String& userRole, const String& userFirstName, const String& userLastName,
        const String& patientUser)
        : role(userRole),
          firstName(userFirstName),
          lastName(userLastName),
          patientUsername(patientUser) {}

    bool isCarePartner() const { return role == "CARE_PARTNER" || role == "CARE_PARTNER_OUS"; }
};

class BGSourceMedtronic : public BGSource {
public:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;

private:
    // Token management
    std::optional<MedtronicTokenData> getTokenData(bool refresh = true);
    bool saveTokenToSettings(const MedtronicTokenData& tokenData);
    std::optional<MedtronicTokenData> refreshToken(const MedtronicTokenData& expiredToken);

    // Configuration discovery with caching
    std::optional<MedtronicConfig> getConfig();
    std::optional<JsonDocument> fetchDiscoveryDocument();
    std::optional<String> fetchRefreshTokenUrl(const String& ssoConfigUrl);

    // User and patient data fetching with caching
    std::optional<MedtronicUserInfo> getUserInfo();
    std::optional<JsonDocument> fetchUserData(
        const MedtronicTokenData& tokenData, const MedtronicConfig& config);
    std::optional<JsonDocument> fetchPatientData(
        const MedtronicTokenData& tokenData, const MedtronicConfig& config);

    // Data fetching
    std::optional<std::list<GlucoseReading>> fetchRecentData(const MedtronicTokenData& tokenData);
    unsigned long long timestampToEpoch(String timestamp) const;

    // Helper functions
    void setCommonHeaders(HTTPClient* client) const;
    void initiateHttpsCall(HTTPClient* client, const String& url);

    // Parse JSON token data from settings and create complete MedtronicTokenData object
    std::optional<MedtronicTokenData> parseTokenFromSettings() const;

    // Constants
    static const unsigned long REQUEST_TIMEOUT_MS;
    static const unsigned long DEFAULT_CACHE_DURATION;

    // Cached data
    static CachedData<MedtronicConfig> configCache;
    static CachedData<MedtronicUserInfo> userInfoCache;

    // Static token data (manages its own expiration)
    static std::optional<MedtronicTokenData> staticTokenData;
};

#endif  // BGSOURCEMEDTRONIC_H

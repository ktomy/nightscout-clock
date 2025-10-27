#include "BGSourceMedtronic.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ServerManager.h"
#include "SettingsManager.h"
#include "globals.h"

// Constants
const unsigned long BGSourceMedtronic::REQUEST_TIMEOUT_MS = 30000;                    // 30 seconds
const unsigned long BGSourceMedtronic::DEFAULT_CACHE_DURATION = 24 * 60 * 60 * 1000;  // 24 hours

// Static cache variables
CachedData<MedtronicConfig> BGSourceMedtronic::configCache(BGSourceMedtronic::DEFAULT_CACHE_DURATION);
CachedData<MedtronicUserInfo> BGSourceMedtronic::userInfoCache(
    BGSourceMedtronic::DEFAULT_CACHE_DURATION);

// Static token data
std::optional<MedtronicTokenData> BGSourceMedtronic::staticTokenData;

std::list<GlucoseReading> BGSourceMedtronic::updateReadings(std::list<GlucoseReading> existingReadings) {
    DEBUG_PRINTLN("Medtronic: Starting update readings");

    // Get valid access token (handles loading, refresh, and saving automatically)
    std::optional<MedtronicTokenData> tokenData = getTokenData();
    if (!tokenData) {
        DEBUG_PRINTLN("Medtronic: No valid token data found. Please authenticate first.");
        DisplayManager.showFatalError(
            String("Medtronic: No valid token data found. Please authenticate first."));
    }

    // Fetch recent data / parse glucose readings
    std::optional<std::list<GlucoseReading>> newReadingsOptional = fetchRecentData(tokenData.value());
    if (!newReadingsOptional) {
        DEBUG_PRINTLN("Medtronic: Failed to fetch recent data");
        return existingReadings;
    }

    std::list<GlucoseReading> newReadings = newReadingsOptional.value();

    // // Merge with existing readings (remove duplicates and keep chronological order)
    // for (const auto& newReading : newReadings) {
    //     bool found = false;
    //     for (const auto& existingReading : existingReadings) {
    //         if (abs((long long)existingReading.epoch - (long long)newReading.epoch) <= 60) {
    //             found = true;
    //             break;
    //         }
    //     }
    //     if (!found) {
    //         existingReadings.push_front(newReading);
    //     }
    // }

    // // Sort by epoch and remove old readings
    // existingReadings.sort(
    //     [](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });

    DEBUG_PRINTLN("Medtronic: Successfully updated " + String(newReadings.size()) + " new readings");
    return newReadings;
}

std::optional<MedtronicTokenData> BGSourceMedtronic::getTokenData(bool refresh) {
    // First, ensure we have token data - load from settings if we don't have it
    if (!staticTokenData) {
        DEBUG_PRINTLN("Medtronic: Loading token from settings");
        staticTokenData = parseTokenFromSettings();
        if (!staticTokenData) {
            DEBUG_PRINTLN("Medtronic: No valid token found in settings");
            return std::nullopt;
        }
    }

    // Now check if the token is expired and refresh if needed (only if refresh is enabled)
    if (refresh && staticTokenData->isExpired()) {
        DEBUG_PRINTLN("Medtronic: Token expired, refreshing");
        staticTokenData = refreshToken(staticTokenData.value());
        if (!staticTokenData) {
            DEBUG_PRINTLN("Medtronic: Token refresh failed");
            return std::nullopt;
        }

        // Save refreshed token
        if (!saveTokenToSettings(staticTokenData.value())) {
            DEBUG_PRINTLN("Medtronic: Warning - Failed to save refreshed token");
            return std::nullopt;
        }
    }

    DEBUG_PRINTLN("Medtronic: Using valid token data");
    return staticTokenData;
}

bool BGSourceMedtronic::saveTokenToSettings(const MedtronicTokenData& tokenData) {
    String tokenJson = tokenData.toJson();

    if (tokenJson.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: Failed to serialize token data");
        return false;
    }

    SettingsManager.settings.medtronic_token_json = tokenJson;
    if (!SettingsManager.saveSettingsToFile()) {
        DEBUG_PRINTLN("Medtronic: Failed to save settings to file");
        return false;
    }

    DEBUG_PRINTLN("Medtronic: Token saved successfully");
    return true;
}

std::optional<MedtronicTokenData> BGSourceMedtronic::parseTokenFromSettings() const {
    String tokenJson = SettingsManager.settings.medtronic_token_json;
    if (tokenJson.isEmpty()) {
        return std::nullopt;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, tokenJson);
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse token JSON: " + String(error.c_str()));
        return std::nullopt;
    }

    MedtronicTokenData tokenData(
        doc["client_id"].as<String>(), doc["client_secret"].as<String>(),
        doc["mag-identifier"].as<String>(), doc["scope"].as<String>());

    // Set access token and refresh token, extract convenience fields from JWT payload
    // Note: expiration is automatically extracted from JWT exp claim
    if (!tokenData.setAccessToken(doc["access_token"].as<String>(), doc["refresh_token"].as<String>())) {
        DEBUG_PRINTLN("Medtronic: Failed to decode access token payload");
        return std::nullopt;
    }

    return tokenData;
}

std::optional<MedtronicTokenData> BGSourceMedtronic::refreshToken(
    const MedtronicTokenData& expiredToken) {
    DEBUG_PRINTLN("Medtronic: Refreshing access token");

    // Get configuration for token endpoint
    std::optional<MedtronicConfig> config = getConfig();
    if (!config) {
        DEBUG_PRINTLN("Medtronic: Failed to get configuration for token refresh");
        return std::nullopt;
    }

    // Prepare refresh request
    initiateHttpsCall(client, config->token_url);
    client->addHeader("Content-Type", "application/x-www-form-urlencoded");
    client->addHeader("Accept", "application/json");
    client->addHeader("mag-identifier", expiredToken.getMagIdentifier());
    client->setUserAgent("Dalvik/2.1.0 (Linux; U; Android 10; Nexus 5X Build/QQ3A.200805.001)");

    // Build form-encoded refresh request payload
    String postData = "refresh_token=" + expiredToken.getRefreshToken() +
                      "&client_id=" + expiredToken.getClientId() +
                      "&client_secret=" + expiredToken.getClientSecret() + "&grant_type=refresh_token";

    // Execute refresh request
    int httpCode = client->POST(postData);
    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Medtronic: Token refresh failed with HTTP " + String(httpCode));
        client->end();
        return std::nullopt;
    }

    String response = client->getString();
    client->end();

    // Parse refresh response
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse refresh response: " + String(error.c_str()));
        return std::nullopt;
    }

    // Validate response contains required tokens
    if (!responseDoc["access_token"].is<String>() || !responseDoc["refresh_token"].is<String>()) {
        DEBUG_PRINTLN("Medtronic: Refresh response missing required tokens");
        return std::nullopt;
    }

    // Build refreshed token data with same configuration
    MedtronicTokenData refreshedToken(
        expiredToken.getClientId(), expiredToken.getClientSecret(), expiredToken.getMagIdentifier(),
        expiredToken.getScope());

    // Set both access token and refresh token, extract convenience fields from JWT payload
    // Note: expiration is automatically extracted from JWT exp claim
    if (!refreshedToken.setAccessToken(
            responseDoc["access_token"].as<String>(), responseDoc["refresh_token"].as<String>())) {
        DEBUG_PRINTLN("Medtronic: Failed to decode refreshed access token payload");
        return std::nullopt;
    }

    DEBUG_PRINTLN("Medtronic: Token refresh successful");
    return refreshedToken;
}

std::optional<MedtronicConfig> BGSourceMedtronic::getConfig() {
    // Check cache first
    std::optional<MedtronicConfig> cachedConfig = configCache.get();
    if (cachedConfig) {
        DEBUG_PRINTLN("Medtronic: Using cached configuration");
        return cachedConfig.value();
    }

    // // Get country from token data for region-specific configuration
    // std::optional<MedtronicTokenData> tokenData = getTokenData(false);
    // if (!tokenData) {
    //     DEBUG_PRINTLN("Medtronic: No valid token data available for configuration");
    //     return std::nullopt;
    // }
    //
    // String country = tokenData->getAccessTokenCountry();
    // DEBUG_PRINTLN(
    //     "Medtronic: No cached configuration or expired, fetching fresh config for country: " + country);

    std::optional<JsonDocument> discoveryDocOptional = fetchDiscoveryDocument();
    if (!discoveryDocOptional) {
        return std::nullopt;
    }

    JsonDocument discoveryDoc = discoveryDocOptional.value();

    // Find region for the country - default to EU region
    String region = "EU";

    // String country = "";
    // if (!country.isEmpty()) {
    //     JsonArray supportedCountries = discoveryDoc["supportedCountries"];
    //     for (JsonVariant countryVariant : supportedCountries) {
    //         JsonObject countryObj = countryVariant.as<JsonObject>();
    //         String upperCountry = country;
    //         upperCountry.toUpperCase();
    //         if (countryObj[upperCountry.c_str()].is<JsonObject>()) {
    //             region = countryObj[upperCountry.c_str()]["region"].as<String>();
    //             break;
    //         }
    //     }
    // }

    DEBUG_PRINTLN("Medtronic: Found region: " + region);

    // Find configuration for the region
    JsonArray cpArray = discoveryDoc["CP"];
    JsonObject regionConfig;
    for (JsonVariant cpVariant : cpArray) {
        JsonObject cpObj = cpVariant.as<JsonObject>();
        if (cpObj["region"].as<String>() == region) {
            regionConfig = cpObj;
            break;
        }
    }

    if (regionConfig.isNull()) {
        DEBUG_PRINTLN("Medtronic: Failed to get config base URLs for region " + region);
        return std::nullopt;
    }

    // Build configuration URLs - save all base URLs from discovery
    MedtronicConfig config;
    config.region = region;
    config.base_url_carelink = regionConfig["baseUrlCareLink"].as<String>();
    config.base_url_cumulus = regionConfig["baseUrlCumulus"].as<String>();
    config.base_url_pde = regionConfig["baseUrlPde"].as<String>();
    config.base_url_aem = regionConfig["baseUrlAem"].as<String>();

    // Fetch SSO configuration to get token URL
    String ssoConfigUrl = regionConfig["SSOConfiguration"].as<String>();
    if (ssoConfigUrl.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: No SSO configuration URL found for region " + region);
        return std::nullopt;
    }

    std::optional<String> tokenUrl = fetchRefreshTokenUrl(ssoConfigUrl);
    if (!tokenUrl) {
        DEBUG_PRINTLN("Medtronic: Failed to fetch SSO configuration");
        return std::nullopt;
    }

    config.token_url = tokenUrl.value();

    // Cache the configuration
    configCache.set(config);

    DEBUG_PRINTLN("Medtronic: Configuration fetched successfully");
    DEBUG_PRINTLN("Medtronic: Region: " + config.region);
    DEBUG_PRINTLN("Medtronic: CareLink Base URL: " + config.base_url_carelink);
    DEBUG_PRINTLN("Medtronic: Cumulus Base URL: " + config.base_url_cumulus);
    DEBUG_PRINTLN("Medtronic: PDE Base URL: " + config.base_url_pde);
    DEBUG_PRINTLN("Medtronic: AEM Base URL: " + config.base_url_aem);
    DEBUG_PRINTLN("Medtronic: Token URL: " + config.token_url);

    return config;
}

std::optional<JsonDocument> BGSourceMedtronic::fetchDiscoveryDocument() {
    DEBUG_PRINTLN("Medtronic: Fetching configuration from discovery endpoint");

    // Always use the EU discovery endpoint
    String discoveryUrl = "https://clcloud.minimed.eu/connect/carepartner/v11/discover/android/3.3";
    DEBUG_PRINTLN("Medtronic: Discovery URL: " + discoveryUrl);

    initiateHttpsCall(client, discoveryUrl);
    setCommonHeaders(client);

    int httpCode = client->GET();
    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Medtronic: Discovery request failed with code: " + String(httpCode));
        client->end();
        return std::nullopt;
    }

    String discoveryResponse = client->getString();
    client->end();

    // Create a filter to only parse the specific fields you need
    JsonDocument filter;
    filter["supportedCountries"] = true;
    filter["CP"] = true;

    // Parse discovery response
    JsonDocument discoveryDoc;

    DeserializationError error = deserializeJson(discoveryDoc, discoveryResponse);
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse discovery response: " + String(error.c_str()));
        return std::nullopt;
    }

    return discoveryDoc;
}

std::optional<String> BGSourceMedtronic::fetchRefreshTokenUrl(const String& ssoConfigUrl) {
    DEBUG_PRINTLN("Medtronic: Fetching refresh token URL from SSO configuration: " + ssoConfigUrl);

    initiateHttpsCall(client, ssoConfigUrl);
    setCommonHeaders(client);

    int httpCode = client->GET();
    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Medtronic: SSO config request failed with code: " + String(httpCode));
        client->end();
        return std::nullopt;
    }

    DEBUG_PRINTLN("Medtronic: SSO config request successful");

    WiFiClient* stream = client->getStreamPtr();

    // Parse SSO configuration response
    JsonDocument ssoDoc;
    DeserializationError error = deserializeJson(ssoDoc, *stream);
    if (error) {
        client->end();
        DEBUG_PRINTLN("Medtronic: Failed to parse SSO config response: " + String(error.c_str()));
        return std::nullopt;
    }
    client->end();

    // Extract server configuration
    if (!ssoDoc["server"].is<JsonObject>() || !ssoDoc["system_endpoints"].is<JsonObject>()) {
        DEBUG_PRINTLN("Medtronic: Invalid SSO configuration structure");
        return std::nullopt;
    }

    JsonObject server = ssoDoc["server"];
    JsonObject systemEndpoints = ssoDoc["system_endpoints"];

    // Build SSO base URL: https://hostname:port/prefix
    String hostname = server["hostname"].as<String>();
    int port = server["port"] | 443;  // Default to 443 if not specified
    String prefix = server["prefix"].as<String>();
    String tokenEndpointPath = systemEndpoints["token_endpoint_path"].as<String>();

    if (hostname.isEmpty() || tokenEndpointPath.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: Missing required SSO configuration fields");
        return std::nullopt;
    }

    String ssoBaseUrl = "https://" + hostname;
    if (port != 443) {
        ssoBaseUrl += ":" + String(port);
    }
    if (!prefix.isEmpty()) {
        ssoBaseUrl += "/" + prefix;
    }

    String tokenUrl = ssoBaseUrl + tokenEndpointPath;

    DEBUG_PRINTLN("Medtronic: Refresh token URL extracted successfully");
    DEBUG_PRINTLN("Medtronic: Token URL: " + tokenUrl);

    return tokenUrl;
}

std::optional<MedtronicUserInfo> BGSourceMedtronic::getUserInfo() {
    // Check cache first
    std::optional<MedtronicUserInfo> cachedUserInfo = userInfoCache.get();
    if (cachedUserInfo) {
        DEBUG_PRINTLN("Medtronic: Using cached user info");
        return cachedUserInfo.value();
    }

    DEBUG_PRINTLN("Medtronic: No cached user info or expired, fetching fresh user info");

    // Get token data and config when we need to fetch fresh data
    std::optional<MedtronicTokenData> tokenData = getTokenData();
    if (!tokenData) {
        DEBUG_PRINTLN("Medtronic: No token data available");
        return std::nullopt;
    }

    std::optional<MedtronicConfig> config = getConfig();
    if (!config) {
        DEBUG_PRINTLN("Medtronic: No config available");
        return std::nullopt;
    }

    // Fetch user data
    std::optional<JsonDocument> userData = fetchUserData(tokenData.value(), config.value());
    if (!userData) {
        DEBUG_PRINTLN("Medtronic: Failed to fetch user data");
        return std::nullopt;
    }

    // Extract required fields from user data
    String role = userData.value()["role"].as<String>();
    String firstName = userData.value()["firstName"].as<String>();
    String lastName = userData.value()["lastName"].as<String>();

    // Validate required fields
    if (role.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: No role found in user data");
        return std::nullopt;
    }
    if (firstName.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: No first name found in user data");
        return std::nullopt;
    }
    if (lastName.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: No last name found in user data");
        return std::nullopt;
    }

    // Create user info using the patient constructor initially
    MedtronicUserInfo userInfo(role, firstName, lastName);

    // If user is a care partner, also fetch patient data and recreate with care partner constructor
    if (userInfo.isCarePartner()) {
        DEBUG_PRINTLN("Medtronic: User is care partner, fetching patient data");
        std::optional<JsonDocument> patientData = fetchPatientData(tokenData.value(), config.value());
        if (!patientData) {
            DEBUG_PRINTLN("Medtronic: Failed to fetch patient data for care partner");
            return std::nullopt;
        }

        String patientUsername = patientData.value()["username"].as<String>();

        // Validate required patient fields
        if (patientUsername.isEmpty()) {
            DEBUG_PRINTLN("Medtronic: No patient username found for care partner");
            return std::nullopt;
        }

        // Create new userInfo with patient data using the care partner constructor
        userInfo = MedtronicUserInfo(role, firstName, lastName, patientUsername);
    }

    // Cache the user info
    userInfoCache.set(userInfo);

    DEBUG_PRINTLN("Medtronic: User info fetched successfully");
    DEBUG_PRINTLN("Medtronic: Role: " + userInfo.role);
    DEBUG_PRINTLN("Medtronic: Name: " + userInfo.firstName + " " + userInfo.lastName);
    if (userInfo.patientUsername.has_value()) {
        DEBUG_PRINTLN("Medtronic: Patient username: " + userInfo.patientUsername.value());
    }
    return userInfo;
}

std::optional<JsonDocument> BGSourceMedtronic::fetchUserData(
    const MedtronicTokenData& tokenData, const MedtronicConfig& config) {
    String url = config.base_url_carelink + "/users/me";
    DEBUG_PRINTLN("Medtronic: Fetching user data from: " + url);

    initiateHttpsCall(client, url);
    setCommonHeaders(client);
    client->addHeader("Authorization", "Bearer " + tokenData.getAccessToken());
    client->addHeader("mag-identifier", tokenData.getMagIdentifier());

    int httpCode = client->GET();

    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Medtronic: User data request failed with code: " + String(httpCode));
        client->end();
        return std::nullopt;
    }

    String response = client->getString();
    client->end();

    // Parse the response
    JsonDocument userData;
    DeserializationError error = deserializeJson(userData, response);
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse user data response: " + String(error.c_str()));
        return std::nullopt;
    }

    DEBUG_PRINTLN("Medtronic: User data fetched successfully");
    return userData;
}

std::optional<JsonDocument> BGSourceMedtronic::fetchPatientData(
    const MedtronicTokenData& tokenData, const MedtronicConfig& config) {
    String url = config.base_url_carelink + "/links/patients";
    DEBUG_PRINTLN("Medtronic: Fetching patient data from: " + url);

    initiateHttpsCall(client, url);
    setCommonHeaders(client);
    client->addHeader("Authorization", "Bearer " + tokenData.getAccessToken());
    client->addHeader("mag-identifier", tokenData.getMagIdentifier());

    int httpCode = client->GET();

    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Medtronic: Patient data request failed with code: " + String(httpCode));
        client->end();
        return std::nullopt;
    }

    String response = client->getString();
    client->end();

    // Parse the response - patient data is returned as an array, we take the first element
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse patient data response: " + String(error.c_str()));
        return std::nullopt;
    }

    // Extract first patient from array
    if (responseDoc.is<JsonArray>() && responseDoc.size() > 0) {
        JsonDocument patientData;
        patientData.set(responseDoc[0]);
        DEBUG_PRINTLN("Medtronic: Patient data fetched successfully");
        return patientData;
    }

    DEBUG_PRINTLN("Medtronic: No patient data found in response");
    return std::nullopt;
}

std::optional<std::list<GlucoseReading>> BGSourceMedtronic::fetchRecentData(const MedtronicTokenData& tokenData) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Medtronic: Fetching recent data");
#endif

    std::optional<MedtronicConfig> config = getConfig();
    if (!config) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Medtronic: Failed to get configuration for data fetch");
#endif
        DisplayManager.showFatalError(String("Medtronic: Could not load config."));
    }

    // Get user info (will use cached user info if available)
    std::optional<MedtronicUserInfo> userInfo = getUserInfo();
    if (!userInfo) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Medtronic: Failed to get user info for data fetch");
#endif
        DisplayManager.showFatalError(String("Medtronic: Could not load user info."));
    }

    // Build the API endpoint URL using discovered configuration
    String url = config->base_url_cumulus + "/display/message";
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Medtronic: API URL: " + url);
#endif

    // Build POST data
    JsonDocument postData;
    postData["username"] = tokenData.getAccessTokenPreferredUsername();
    postData["role"] = "patient";

    if (userInfo->isCarePartner()) {
        postData["role"] = "carepartner";
        postData["patientId"] = userInfo->patientUsername.value();
    }

    String postDataString;
    serializeJson(postData, postDataString);
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("Medtronic: POST data: " + postDataString);
#endif

    initiateHttpsCall(client, url);
    setCommonHeaders(client);
    client->addHeader("Authorization", "Bearer " + tokenData.getAccessToken());
    client->addHeader("mag-identifier", tokenData.getMagIdentifier());

    int httpCode = client->POST(postDataString);

    if (httpCode != HTTP_CODE_OK) {
#ifdef DEBUG_BG_SOURCE
        if (httpCode == HTTP_CODE_UNAUTHORIZED) {
            DEBUG_PRINTLN("Medtronic: Unauthorized - token may be invalid");
        } else {
            DEBUG_PRINTLN("Medtronic: HTTP request failed with code: " + String(httpCode));
        }
#endif

        client->end();
        return std::nullopt;
    }

    // Get the stream instead of loading entire JSON into memory
    WiFiClient* stream = client->getStreamPtr();

    JsonDocument filter;
    filter["patientData"]["sgs"][0]["sg"] = true;
    filter["patientData"]["sgs"][0]["timestamp"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

    if (error) {
        Serial.print(F("JSON parsing failed: "));
        Serial.println(error.c_str());
        client->end();
        return std::nullopt;
    }

    std::list<GlucoseReading> readings;

    unsigned long long currentTime = ServerManager.getUtcEpoch();
    for (JsonVariant sg : doc["patientData"]["sgs"].as<JsonArray>()) {
#ifdef DEBUG_BG_SOURCE
        DEBUG_PRINTLN("Medtronic: Parsing SG entry:");
        for (JsonPair keyValue : sg.as<JsonObject>()) {
            DEBUG_PRINTLN("  " + String(keyValue.key().c_str()) + ": " + String(keyValue.value().as<String>()));
        }
#endif

        GlucoseReading reading;
        reading.sgv = sg["sg"].as<int>();
        if (reading.sgv <= 0) {
            continue;
        }

        reading.trend = BG_TREND::NONE;
        reading.epoch = timestampToEpoch(sg["timestamp"]);  // Convert to seconds

        // Only add readings of last 3 hours
        if (reading.epoch < (currentTime - BG_BACKFILL_SECONDS)) {
            continue;
        }

        readings.push_front(reading);
    }
    doc.clear();
    client->end();

    firstConnectionSuccess = true;

    // Sort readings by timestamp
    readings.sort([](const GlucoseReading& a, const GlucoseReading& b) { return a.epoch < b.epoch; });

    DEBUG_PRINTLN("Medtronic: Parsed " + String(readings.size()) + " glucose readings");
    return readings;
}

void BGSourceMedtronic::setCommonHeaders(HTTPClient* client) const {
    client->addHeader("Accept", "application/json");
    client->addHeader("Content-Type", "application/json");
    client->setUserAgent("Dalvik/2.1.0 (Linux; U; Android 10; Nexus 5X Build/QQ3A.200805.001)");
}

void BGSourceMedtronic::initiateHttpsCall(HTTPClient* client, const String& url) {
    LCBUrl parsedUrl;
    // if (parsedUrl.setUrl(url)) {
    //     DEBUG_PRINTLN("Medtronic: Initiating HTTPS call to " + parsedUrl.getHost() + ":" +
    //                    String(parsedUrl.getPort()) + String("/") + parsedUrl.getPath() + parsedUrl.getAfterPath());
    //     client->begin(
    //         *wifiSecureClient, parsedUrl.getHost(), parsedUrl.getPort(),
    //         String("/") + parsedUrl.getPath() + parsedUrl.getAfterPath(), true);
    // } else {
        // Fallback to direct URL if parsing fails
        client->begin(*wifiSecureClient, url);
    // }
}

unsigned long long BGSourceMedtronic::timestampToEpoch(String timestamp) const {
    if (timestamp.c_str() == nullptr) {
        return 0;
    }

    tm tm;
    strptime(timestamp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTF(
        "Value to parse: %s, Parsed time: %02d:%02d:%02d, date: %02d/%02d/%04d\n", timestamp.c_str(),
        tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
#endif
    time_t t = mktime(&tm);
    return t;
}

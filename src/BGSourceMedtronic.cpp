#include "BGSourceMedtronic.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "SettingsManager.h"
#include "ServerManager.h"
#include "globals.h"

// Constants
const unsigned long BGSourceMedtronic::TOKEN_REFRESH_MARGIN_MS = 5 * 60 * 1000; // 5 minutes
const unsigned long BGSourceMedtronic::REQUEST_TIMEOUT_MS = 30000; // 30 seconds

std::list<GlucoseReading> BGSourceMedtronic::updateReadings(std::list<GlucoseReading> existingReadings) {
    DEBUG_PRINTLN("Medtronic: Starting update readings");
    
    // Load token data from storage
    MedtronicTokenData tokenData;
    if (!loadTokenData(tokenData)) {
        DEBUG_PRINTLN("Medtronic: No valid token data found. Please authenticate first.");
        return existingReadings;
    }
    
    // Check if token needs refresh
    if (tokenData.isExpired()) {
        DEBUG_PRINTLN("Medtronic: Token expired, attempting refresh");
        if (!refreshAccessToken(tokenData)) {
            DEBUG_PRINTLN("Medtronic: Failed to refresh token");
            return existingReadings;
        }
        saveTokenData(tokenData);
    }
    
    // Fetch recent data
    String jsonData;
    if (!fetchRecentData(tokenData, jsonData)) {
        DEBUG_PRINTLN("Medtronic: Failed to fetch recent data");
        return existingReadings;
    }
    
    // Parse glucose readings
    std::list<GlucoseReading> newReadings = parseGlucoseReadings(jsonData);
    
    // Merge with existing readings (remove duplicates and keep chronological order)
    for (const auto& newReading : newReadings) {
        bool found = false;
        for (const auto& existingReading : existingReadings) {
            if (abs((long long)existingReading.epoch - (long long)newReading.epoch) <= 60) {
                found = true;
                break;
            }
        }
        if (!found) {
            existingReadings.push_back(newReading);
        }
    }
    
    // Sort by epoch and remove old readings
    existingReadings.sort([](const GlucoseReading& a, const GlucoseReading& b) {
        return a.epoch < b.epoch;
    });
    
    DEBUG_PRINTLN("Medtronic: Successfully updated " + String(newReadings.size()) + " new readings");
    return existingReadings;
}

bool BGSourceMedtronic::loadTokenData(MedtronicTokenData& tokenData) {
    // Load token data from settings JSON string instead of file
    String tokenJson = SettingsManager.settings.medtronic_token_json;
    
    if (tokenJson.isEmpty()) {
        DEBUG_PRINTLN("Medtronic: No token JSON found in settings");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, tokenJson);
    
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse token JSON from settings: " + String(error.c_str()));
        return false;
    }
    
    tokenData.access_token = doc["access_token"].as<String>();
    tokenData.refresh_token = doc["refresh_token"].as<String>();
    tokenData.client_id = doc["client_id"].as<String>();
    tokenData.client_secret = doc["client_secret"].as<String>();
    tokenData.mag_identifier = doc["mag-identifier"].as<String>();
    tokenData.scope = doc["scope"].as<String>();
    tokenData.expires_at = doc["expires_at"] | 0;
    
    if (!tokenData.isValid()) {
        DEBUG_PRINTLN("Medtronic: Invalid token data structure");
        return false;
    }
    
    DEBUG_PRINTLN("Medtronic: Token data loaded successfully");
    return true;
}

bool BGSourceMedtronic::saveTokenData(const MedtronicTokenData& tokenData) {
    JsonDocument doc;
    doc["access_token"] = tokenData.access_token;
    doc["refresh_token"] = tokenData.refresh_token;
    doc["client_id"] = tokenData.client_id;
    doc["client_secret"] = tokenData.client_secret;
    doc["mag-identifier"] = tokenData.mag_identifier;
    doc["scope"] = tokenData.scope;
    doc["expires_at"] = tokenData.expires_at;
    
    String tokenJson;
    if (serializeJson(doc, tokenJson) == 0) {
        DEBUG_PRINTLN("Medtronic: Failed to serialize token data");
        return false;
    }
    
    // Save to settings
    SettingsManager.settings.medtronic_token_json = tokenJson;
    if (!SettingsManager.saveSettingsToFile()) {
        DEBUG_PRINTLN("Medtronic: Failed to save settings to file");
        return false;
    }
    
    DEBUG_PRINTLN("Medtronic: Token data saved successfully to settings");
    return true;
}

bool BGSourceMedtronic::refreshAccessToken(MedtronicTokenData& tokenData) {
    // Note: In a real implementation, you would need to discover the endpoints
    // For now, we'll use a simplified approach assuming EU region
    String country = SettingsManager.settings.medtronic_country.isEmpty() ? 
                     "eu" : SettingsManager.settings.medtronic_country;
    
    String discoveryUrl = getDiscoveryUrl(country);
    
    // For this implementation, we'll assume the user has a valid refresh token
    // and manually configured the endpoints in the token file
    // A complete implementation would require full OAuth2 flow
    
    DEBUG_PRINTLN("Medtronic: Token refresh not fully implemented - requires manual token management");
    return false;
}

bool BGSourceMedtronic::fetchRecentData(const MedtronicTokenData& tokenData, String& jsonData) {
    // Construct the API endpoint URL
    // Note: This is simplified - actual endpoint discovery would be more complex
    String baseUrl;
    String country = SettingsManager.settings.medtronic_country.isEmpty() ? 
                     "eu" : SettingsManager.settings.medtronic_country;
    
    if (country == "us") {
        baseUrl = "https://clcloud.minimed.com";
    } else {
        baseUrl = "https://clcloud.minimed.eu";
    }
    
    String url = baseUrl + "/connect/carepartner/v6/patients/me/data";
    
    // Make authenticated request
    client->begin(*wifiSecureClient, url);
    client->setTimeout(REQUEST_TIMEOUT_MS);
    
    // Set authentication headers
    client->addHeader("Authorization", "Bearer " + tokenData.access_token);
    client->addHeader("mag-identifier", tokenData.mag_identifier);
    client->addHeader("Accept", "application/json");
    client->addHeader("User-Agent", "nightscout-clock/1.0");
    
    int httpCode = client->GET();
    
    if (httpCode == HTTP_CODE_OK) {
        jsonData = client->getString();
        client->end();
        DEBUG_PRINTLN("Medtronic: Successfully fetched data (" + String(jsonData.length()) + " bytes)");
        return true;
    } else if (httpCode == HTTP_CODE_UNAUTHORIZED) {
        DEBUG_PRINTLN("Medtronic: Unauthorized - token may be invalid");
    } else {
        DEBUG_PRINTLN("Medtronic: HTTP request failed with code: " + String(httpCode));
    }
    
    client->end();
    return false;
}

std::list<GlucoseReading> BGSourceMedtronic::parseGlucoseReadings(const String& jsonData) {
    std::list<GlucoseReading> readings;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        DEBUG_PRINTLN("Medtronic: Failed to parse response JSON: " + String(error.c_str()));
        return readings;
    }
    
    // Navigate to the glucose data in the response
    if (!doc["patientData"].is<JsonObject>()) {
        DEBUG_PRINTLN("Medtronic: No patientData found in response");
        return readings;
    }
    
    JsonObject patientData = doc["patientData"];
    
    // Parse sensor glucose data
    if (patientData["sgs"].is<JsonArray>()) {
        JsonArray sgsArray = patientData["sgs"];
        
        for (JsonVariant sgValue : sgsArray) {
            JsonObject sg = sgValue.as<JsonObject>();
            
            if (sg.containsKey("sg") && sg.containsKey("datetime")) {
                GlucoseReading reading;
                
                // Extract glucose value (convert from mmol/L to mg/dL if needed)
                float sgValue = sg["sg"].as<float>();
                reading.sgv = (int)(sgValue * 18.0182); // Convert mmol/L to mg/dL
                
                // Extract timestamp - Medtronic uses milliseconds since epoch
                unsigned long long timestamp = sg["datetime"].as<unsigned long long>();
                reading.epoch = timestamp / 1000; // Convert to seconds
                
                // Extract trend
                String trendStr = sg["trendArrow"].as<String>();
                reading.trend = parseTrendArrow(trendStr);
                
                // Only add valid readings (not older than 24 hours)
                unsigned long long currentTime = ServerManager.getUtcEpoch();
                if (reading.sgv > 0 && reading.epoch > (currentTime - 86400)) {
                    readings.push_back(reading);
                }
            }
        }
    }
    
    // Also parse CGM data if available
    if (patientData["cgmInfo"].is<JsonObject>()) {
        JsonObject cgmInfo = patientData["cgmInfo"];
        
        if (cgmInfo.containsKey("calibrationStatus") && 
            cgmInfo.containsKey("lastSGTrend") && 
            cgmInfo.containsKey("lastSG")) {
            
            GlucoseReading reading;
            reading.sgv = cgmInfo["lastSG"]["sg"].as<int>();
            reading.epoch = cgmInfo["lastSG"]["datetime"].as<unsigned long long>() / 1000;
            reading.trend = parseTrendArrow(cgmInfo["lastSGTrend"]["direction"].as<String>());
            
            unsigned long long currentTime = ServerManager.getUtcEpoch();
            if (reading.sgv > 0 && reading.epoch > (currentTime - 86400)) {
                readings.push_back(reading);
            }
        }
    }
    
    // Sort readings by timestamp
    readings.sort([](const GlucoseReading& a, const GlucoseReading& b) {
        return a.epoch < b.epoch;
    });
    
    DEBUG_PRINTLN("Medtronic: Parsed " + String(readings.size()) + " glucose readings");
    return readings;
}

String BGSourceMedtronic::getDiscoveryUrl(const String& country) {
    if (country == "us") {
        return "https://clcloud.minimed.com/connect/carepartner/v11/discover/android/3.2";
    } else {
        return "https://clcloud.minimed.eu/connect/carepartner/v11/discover/android/3.2";
    }
}

BG_TREND BGSourceMedtronic::parseTrendArrow(const String& trend) {
    if (trend == "UP_FAST" || trend == "DoubleUp" || trend == "2") {
        return BG_TREND::DOUBLE_UP;
    } else if (trend == "UP" || trend == "SingleUp" || trend == "1") {
        return BG_TREND::SINGLE_UP;
    } else if (trend == "UP_SLOW" || trend == "FortyFiveUp" || trend == "3") {
        return BG_TREND::FORTY_FIVE_UP;
    } else if (trend == "FLAT" || trend == "Flat" || trend == "4") {
        return BG_TREND::FLAT;
    } else if (trend == "DOWN_SLOW" || trend == "FortyFiveDown" || trend == "5") {
        return BG_TREND::FORTY_FIVE_DOWN;
    } else if (trend == "DOWN" || trend == "SingleDown" || trend == "6") {
        return BG_TREND::SINGLE_DOWN;
    } else if (trend == "DOWN_FAST" || trend == "DoubleDown" || trend == "7") {
        return BG_TREND::DOUBLE_DOWN;
    } else {
        return BG_TREND::NONE;
    }
}
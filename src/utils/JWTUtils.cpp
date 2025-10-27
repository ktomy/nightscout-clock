#include "JWTUtils.h"

#include <Arduino.h>
#include <mbedtls/base64.h>

#include <vector>

namespace JWTUtils {

String base64UrlDecode(const String& input) {
    if (input.isEmpty()) {
        return "";
    }

    // Convert base64url to standard base64
    String base64 = input;
    base64.replace('-', '+');
    base64.replace('_', '/');

    // Add padding if needed
    int padding = (4 - (base64.length() % 4)) % 4;
    for (int i = 0; i < padding; i++) {
        base64 += '=';
    }

    // Use mbedTLS
    size_t outputLength = 0;

    // First call to get required buffer size
    int result = mbedtls_base64_decode(
        nullptr, 0, &outputLength, (const unsigned char*)base64.c_str(), base64.length());

    if (result != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        // Invalid base64
        return "";
    }

    // Allocate buffer and decode
    std::vector<unsigned char> buffer(outputLength);
    result = mbedtls_base64_decode(
        buffer.data(), buffer.size(), &outputLength, (const unsigned char*)base64.c_str(),
        base64.length());

    if (result != 0) {
        // Decoding failed
        return "";
    }

    return String((char*)buffer.data(), outputLength);
}

std::optional<JsonDocument> getJWTPayload(const String& jwt) {
    if (!isValidJWTFormat(jwt)) {
        return std::nullopt;
    }

    // Split JWT into parts (header.payload.signature)
    int firstDot = jwt.indexOf('.');
    int secondDot = jwt.indexOf('.', firstDot + 1);

    // Extract payload (between first and second dot)
    String payload = jwt.substring(firstDot + 1, secondDot);
    if (payload.isEmpty()) {
        return std::nullopt;
    }

    // Decode the payload
    String decodedPayload = base64UrlDecode(payload);
    if (decodedPayload.isEmpty()) {
        return std::nullopt;
    }

    // Parse JSON payload
    JsonDocument payloadDoc;
    DeserializationError error = deserializeJson(payloadDoc, decodedPayload);

    if (error) {
        return std::nullopt;
    }

    return payloadDoc;
}

bool isValidJWTFormat(const String& jwt) {
    if (jwt.isEmpty()) {
        return false;
    }

    // JWT should have exactly 2 dots (3 parts: header.payload.signature)
    int firstDot = jwt.indexOf('.');
    if (firstDot == -1) {
        return false;
    }

    int secondDot = jwt.indexOf('.', firstDot + 1);
    if (secondDot == -1) {
        return false;
    }

    // Ensure no more dots after the second one
    int thirdDot = jwt.indexOf('.', secondDot + 1);
    if (thirdDot != -1) {
        return false;
    }

    // Check that all parts are non-empty
    if (firstDot == 0 || secondDot == firstDot + 1 || secondDot == jwt.length() - 1) {
        return false;
    }

    return true;
}

}  // namespace JWTUtils

#ifndef JWTUTILS_H
#define JWTUTILS_H

#include <ArduinoJson.h>

#include <optional>

/**
 * @brief JWT (JSON Web Token) utility functions
 *
 * This namespace provides utilities for working with JWT tokens,
 * including payload extraction and base64url decoding.
 */
namespace JWTUtils {

/**
 * @brief Decode base64url encoded string to regular string
 *
 * Base64url is a variant of base64 encoding that uses URL-safe characters
 * and omits padding characters.
 *
 * @param input The base64url encoded string
 * @return Decoded string, or empty string if decoding fails
 */
String base64UrlDecode(const String& input);

/**
 * @brief Extract and parse JWT payload from a JWT token
 *
 * Extracts the payload section from a JWT token (format: header.payload.signature)
 * and parses it as JSON.
 *
 * @param jwt The JWT token string
 * @return std::optional<JsonDocument> containing the parsed payload, or std::nullopt if parsing fails
 */
std::optional<JsonDocument> getJWTPayload(const String& jwt);

/**
 * @brief Validate JWT token structure (basic format validation only)
 *
 * Performs basic validation of JWT token format (3 parts separated by dots).
 * Does NOT validate signature or token expiration.
 *
 * @param jwt The JWT token string
 * @return true if token has valid format, false otherwise
 */
bool isValidJWTFormat(const String& jwt);

}  // namespace JWTUtils

#endif  // JWTUTILS_H

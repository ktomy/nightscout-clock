#ifndef CACHEDDATA_H
#define CACHEDDATA_H

#include <Arduino.h>

#include <optional>

/**
 * @brief Generic cached data wrapper with automatic expiration
 *
 * This template class provides automatic caching with configurable expiration times.
 * Uses Arduino's millis() function for timing.
 *
 * @tparam T The type of data to be cached
 */
template <typename T>
class CachedData {
private:
    std::optional<T> data;
    unsigned long cacheTime = 0;
    unsigned long cacheDurationMs;

public:
    /**
     * @brief Construct a new empty CachedData object
     * @param durationMs Cache duration in milliseconds (default: 24 hours)
     */
    explicit CachedData(unsigned long durationMs = 24 * 60 * 60 * 1000) : cacheDurationMs(durationMs) {}

    /**
     * @brief Construct a new CachedData object with initial data
     * @param initialData Initial data to cache
     * @param durationMs Cache duration in milliseconds (default: 24 hours)
     */
    CachedData(const T& initialData, unsigned long durationMs = 24 * 60 * 60 * 1000)
        : data(initialData), cacheTime(millis()), cacheDurationMs(durationMs) {}

    /**
     * @brief Check if cached data is still valid (not expired)
     * @return true if data exists and has not expired, false otherwise
     */
    bool isValid() const {
        return data.has_value() && cacheTime > 0 && (millis() - cacheTime) < cacheDurationMs;
    }

    /**
     * @brief Get cached data if still valid
     * @return std::optional<T> containing the data if valid, std::nullopt otherwise
     */
    std::optional<T> get() const { return isValid() ? data : std::nullopt; }

    /**
     * @brief Set new data and update cache timestamp
     * @param newData The data to cache
     */
    void set(const T& newData) {
        data = newData;
        cacheTime = millis();
    }

    /**
     * @brief Set new data with custom cache duration
     * @param newData The data to cache
     * @param durationMs Custom cache duration in milliseconds
     */
    void set(const T& newData, unsigned long durationMs) {
        data = newData;
        cacheTime = millis();
        cacheDurationMs = durationMs;
    }

    /**
     * @brief Clear all cached data
     */
    void clear() {
        data.reset();
        cacheTime = 0;
    }
};

#endif  // CACHEDDATA_H

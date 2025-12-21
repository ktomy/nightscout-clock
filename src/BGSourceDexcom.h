#ifndef BGSOURCEDEXCOM_H
#define BGSOURCEDEXCOM_H

#include "BGSource.h"

#define DEXCOM_APPLICATION_ID "d89443d2-327c-4a6f-89e5-496bbb0317db"
#define DEXCOM_APPLICATION_ID_JAPAN "d8665ade-9673-4e27-9ff6-92db4ce13d13"
#define DEXCOM_GET_ACCOUNT_ID_PATH "/ShareWebServices/Services/General/AuthenticatePublisherAccount"
#define DEXCOM_GET_SESSION_ID_PATH "/ShareWebServices/Services/General/LoginPublisherAccountById"
#define DEXCOM_GET_GLUCOSE_READINGS_PATH \
    "/ShareWebServices/Services/Publisher/ReadPublisherLatestGlucoseValues"
#define DEXCOM_NON_US_SERVER "https://shareous1.dexcom.com"
#define DEXCOM_US_SERVER "https://share1.dexcom.com"
#define DEXCOM_JAPAN_SERVER "https://share.dexcom.jp"

class BGSourceDexcom : public BGSource {
public:
private:
    std::list<GlucoseReading> updateReadings(std::list<GlucoseReading> existingReadings) override;
    std::list<GlucoseReading> updateReadings(
        DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword,
        std::list<GlucoseReading> existingReadings);
    std::list<GlucoseReading> retrieveReadings(
        DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword, int forMinutes,
        int readingsCount);
    String accountId;
    String sessionId;
    String getAccountId(DEXCOM_SERVER dexcomServer, String dexcomUsername, String dexcomPassword);
    String getSessionId(DEXCOM_SERVER dexcomServer, String accountId, String dexcomPassword);
};

#endif  // BGSOURCEDEXCOM_H

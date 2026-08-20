#pragma once
#include <stdint.h>

namespace RfEnvironmentMath {
inline uint8_t percent(uint32_t hits, uint32_t samples) {
    return samples == 0 ? 0 : static_cast<uint8_t>((hits * 100U + samples / 2U) / samples);
}
inline uint8_t ema(uint8_t previous, uint8_t sample, uint8_t alphaPercent) {
    if (alphaPercent > 100) alphaPercent = 100;
    return static_cast<uint8_t>((static_cast<uint16_t>(sample) * alphaPercent +
        static_cast<uint16_t>(previous) * (100U - alphaPercent) + 50U) / 100U);
}
inline uint16_t frequencyMHz(uint8_t channel) { return 2400U + channel; }
inline int8_t wifiChannelForMHz(uint16_t mhz) {
    if (mhz == 2484) return 14;
    if (mhz < 2412 || mhz > 2472) return -1;
    return static_cast<int8_t>((mhz - 2407) / 5);
}
inline int8_t zigbeeChannelForMHz(uint16_t mhz) {
    if (mhz < 2405 || mhz > 2480) return -1;
    const uint16_t offset = mhz - 2405;
    return offset % 5 == 0 ? static_cast<int8_t>(11 + offset / 5) : -1;
}
inline uint8_t classifyScore(uint8_t score) {
    return score <= 20 ? 0 : score <= 40 ? 1 : score <= 60 ? 2 : score <= 80 ? 3 : 4;
}
inline uint8_t interferenceScore(uint8_t occupancy, uint8_t persistence,
                                 uint8_t burstActivity, uint8_t neighbors) {
    return static_cast<uint8_t>((occupancy * 40U + persistence * 20U +
                                 burstActivity * 20U + neighbors * 20U + 50U) / 100U);
}
inline bool validWindowSeconds(int seconds) { return seconds==1||seconds==5||seconds==10||seconds==30||seconds==60; }
inline bool validRange(int minCh,int maxCh) { return minCh>=0&&maxCh<=125&&minCh<=maxCh; }
inline uint8_t ringNext(uint8_t head,uint8_t capacity) { return capacity ? static_cast<uint8_t>((head+1)%capacity) : 0; }
inline uint8_t burstSeverity(uint8_t delta) { return delta>=50?2:delta>=30?1:0; }
inline int16_t signedDelta(uint8_t before,uint8_t after) { return static_cast<int16_t>(after)-before; }
inline uint8_t protocolRegions(uint16_t mhz) {
    uint8_t flags=0;
    if(mhz>=2401&&mhz<=2484) flags|=1;
    if(mhz==2402||mhz==2426||mhz==2480) flags|=2;
    if(mhz>=2402&&mhz<=2480) flags|=4;
    if(mhz>=2405&&mhz<=2480) flags|=8;
    return flags;
}
}

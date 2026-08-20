#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "core/RfEnvironmentState.h"
#include "core/RfEnvironmentMath.h"
#include "services/RfEnvironmentAnalyzer.h"
#include "drivers/RadioManager.h"
#include "services/RfAuthorizedProbe.h"
using namespace DisplayUi;

void DisplayManager::renderRfEnvironmentScreen() {
    const uint16_t bg=SPECTRUM_CARD_BG, accent=SPECTRUM_ACCENT;
    tft.fillRect(0,14,160,91,ST77XX_BLACK);
    if(appState.appMode==APP_MODE_ENV_OCCUPANCY){
        drawModernHeader("RF ENV OCCUPANCY",accent);uint8_t top[5];rfEnvironmentState.topChannels(top,5);
        tft.setCursor(4,18);tft.setTextColor(ST77XX_GRAY);tft.printf("RANGE %u-%u  WIN %us",rfEnvironmentState.config.minChannel,rfEnvironmentState.config.maxChannel,rfEnvironmentState.config.sampleWindowSeconds);
        tft.setCursor(4,31);tft.setTextColor(ST77XX_WHITE);tft.print("BUSIEST");
        for(int i=0;i<5;i++){tft.setCursor(8+(i%2)*76,43+(i/2)*12);tft.printf("CH%3u %3u%%",top[i],rfEnvironmentState.channels[top[i]].movingAverage);}
        tft.setCursor(4,82);tft.printf("AVG %u%% SCORE %u",rfEnvironmentState.averageOccupancy(),rfEnvironmentState.overallScore());
    } else if(appState.appMode==APP_MODE_ENV_HEATMAP){
        drawModernHeader("TRAFFIC HEATMAP",accent);const uint16_t colors[6]={ST77XX_BLACK,ST77XX_DARKGRAY,SPECTRUM_LOW,SPECTRUM_MID,SPECTRUM_HIGH,SPECTRUM_CRITICAL};
        const int cols=rfEnvironmentState.historyCount;for(int x=0;x<cols;x++){uint8_t bi=(rfEnvironmentState.historyHead+RF_ENV_HISTORY_BUCKETS-cols+x)%RF_ENV_HISTORY_BUCKETS;for(int row=0;row<21;row++){int a=rfEnvironmentState.config.minChannel+row*(rfEnvironmentState.config.maxChannel-rfEnvironmentState.config.minChannel+1)/21,b=rfEnvironmentState.config.minChannel+(row+1)*(rfEnvironmentState.config.maxChannel-rfEnvironmentState.config.minChannel+1)/21;uint16_t sum=0,n=0;for(int ch=a;ch<b;ch++){sum+=rfEnvironmentState.history[bi][ch];n++;}uint8_t v=n?sum/n:0;tft.fillRect(25+x*4,18+(20-row)*4,4,4,colors[v?min(5,1+v/20):0]);}}
        tft.setCursor(2,18);tft.setTextColor(ST77XX_GRAY);tft.print("125");tft.setCursor(8,94);tft.print("0");
    } else if(appState.appMode==APP_MODE_ENV_BURSTS){
        drawModernHeader("BURST MONITOR",SPECTRUM_HIGH);tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE);tft.printf("EVENTS %u",rfEnvironmentState.eventCount);
        if(rfEnvironmentState.eventCount){uint8_t idx=(rfEnvironmentState.eventHead+RF_ENV_BURST_EVENTS-1-envEventScroll)%RF_ENV_BURST_EVENTS;const RfBurstEvent&e=rfEnvironmentState.events[idx];tft.setCursor(4,34);tft.printf("#%lu  CH%u  %u MHz",(unsigned long)e.id,e.channel,e.frequencyMHz);tft.setCursor(4,49);tft.printf("PEAK %u%% BASE %u%%",e.peak,e.baseline);tft.setCursor(4,64);tft.printf("DELTA +%u  DUR %lums",e.delta,(unsigned long)e.durationMs);tft.setCursor(4,80);tft.print(e.severity==RF_BURST_HIGH?"HIGH":e.severity==RF_BURST_MEDIUM?"MEDIUM":"LOW");}
    } else if(appState.appMode==APP_MODE_ENV_COMPARE){
        drawModernHeader("CHANNEL COMPARE",accent);for(int i=0;i<rfEnvironmentState.config.compareCount;i++){uint8_t ch=rfEnvironmentState.config.compareChannels[i];const RfChannelStats&s=rfEnvironmentState.channels[ch];int y=20+i*20;tft.setCursor(3,y);tft.setTextColor(ST77XX_WHITE);tft.printf("CH%3u %3u%% %-4s",ch,s.movingAverage,rfEnvironmentState.scoreLabel(s.score));tft.fillRect(3,y+9,s.movingAverage,5,getSignalColor(s.movingAverage));}
    } else if(appState.appMode==APP_MODE_ENV_STATUS){
        drawModernHeader("RF STATUS",accent);uint8_t score=rfEnvironmentState.overallScore(),top[1];rfEnvironmentState.topChannels(top,1);uint16_t bursts=0;for(int i=0;i<TOTAL_CHANNELS;i++)bursts+=rfEnvironmentState.channels[i].burstCount;tft.setCursor(5,22);tft.setTextColor(ST77XX_WHITE);tft.printf("SCORE %u/100  %s",score,rfEnvironmentState.scoreLabel(score));tft.setCursor(5,42);tft.printf("OCC     %u%%",rfEnvironmentState.averageOccupancy());tft.setCursor(5,56);tft.printf("BURSTS  %u",bursts);tft.setCursor(5,70);tft.printf("PEAK CH %u",top[0]);tft.setCursor(5,84);tft.print("RELATIVE ACTIVITY ONLY");
    } else if(appState.appMode==APP_MODE_ENV_BEFORE_AFTER){
        drawModernHeader("BEFORE / AFTER",accent);const auto&a=rfEnvironmentState.before;const auto&b=rfEnvironmentState.after;tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE);if(!a.valid)tft.print("R: CAPTURE BEFORE");else if(!b.valid)tft.print("R: CAPTURE AFTER");else{tft.printf("AVG %u->%u %+d  PEAK %u->%u",a.average,b.average,(int)b.average-a.average,a.peak,b.peak);tft.setCursor(4,35);tft.printf("BURSTS %u->%u SCORE %u->%u",a.burstCount,b.burstCount,a.score,b.score);tft.setCursor(4,55);tft.printf("CH%u %u%% -> %u%%",envBandChannel,a.channelOccupancy[envBandChannel],b.channelOccupancy[envBandChannel]);tft.setCursor(4,72);tft.printf("DELTA %+d%%",(int)b.channelOccupancy[envBandChannel]-a.channelOccupancy[envBandChannel]);}}
    else if(appState.appMode==APP_MODE_ENV_BAND_INFO){
        drawModernHeader("POSSIBLE OVERLAP",accent);uint16_t f=RfEnvironmentMath::frequencyMHz(envBandChannel);tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE);tft.printf("CH%u  %u MHz",envBandChannel,f);tft.setCursor(4,35);tft.print("POSSIBLE / BAND REGION:");int8_t wifi=RfEnvironmentMath::wifiChannelForMHz(f);tft.setCursor(4,50);if(wifi>0)tft.printf("WiFi CH%d overlap",wifi);else tft.print("WiFi: outside centers");tft.setCursor(4,64);tft.print(f>=2402&&f<=2480?"BLE / BT data region":"BLE / BT: outside");tft.setCursor(4,78);int8_t z=RfEnvironmentMath::zigbeeChannelForMHz(f);if(z>0)tft.printf("Zigbee CH%d center",z);else if(f>=2405&&f<=2480)tft.print("Zigbee band overlap");else tft.print("Zigbee: outside");
    } else {drawModernHeader("AUTHORIZED RF PROBE",SPECTRUM_HIGH);tft.setCursor(34,20);tft.setTextColor(SPECTRUM_HIGH);tft.print("LAB USE ONLY");tft.setCursor(7,36);tft.setTextColor(ST77XX_WHITE);
#if RF_LAB_TX_ENABLED
        const auto&c=rfEnvironmentState.config;tft.printf("CH %u  POWER LOW",c.probeChannel);tft.setCursor(7,50);tft.printf("INTERVAL %ums",c.probeIntervalMs);tft.setCursor(7,64);tft.printf("LIMIT %us / %u pkt",c.probeMaxDurationSeconds,c.probePacketCount);tft.setCursor(7,78);tft.printf("%s SENT %u",rfAuthorizedProbe.isRunning()?"RUNNING":"READY",rfAuthorizedProbe.packetsSent());
#else
        tft.print("RX ONLY BUILD");tft.setCursor(7,54);tft.print("TX code not compiled");
#endif
        tft.setCursor(7,91);tft.print("B = EMERGENCY STOP");}
    drawModernFooter("U/D VIEW",rfEnvironmentState.running?"R STOP":"R START","B BACK");
}

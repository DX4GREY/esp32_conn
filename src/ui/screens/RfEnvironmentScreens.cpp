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
    (void)bg;
    const bool layout = needRedraw;
    auto clearLine = [&](int y) { tft.fillRect(2, y, 156, 9, ST77XX_BLACK); };
    if(appState.appMode==APP_MODE_ENV_OCCUPANCY){
        if(layout){drawModernHeader("RF ENV OCCUPANCY",accent);tft.setCursor(4,31);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);tft.print("BUSIEST");}uint8_t top[5];rfEnvironmentState.topChannels(top,5);
        clearLine(18);tft.setCursor(4,18);tft.setTextColor(ST77XX_GRAY,ST77XX_BLACK);tft.printf("RANGE %u-%u  WIN %us",rfEnvironmentState.config.minChannel,rfEnvironmentState.config.maxChannel,rfEnvironmentState.config.sampleWindowSeconds);
        for(int row=0;row<3;row++)clearLine(43+row*12);
        tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);for(int i=0;i<5;i++){tft.setCursor(8+(i%2)*76,43+(i/2)*12);tft.printf("CH%3u %3u%%",top[i],rfEnvironmentState.channels[top[i]].movingAverage);}
        clearLine(82);tft.setCursor(4,82);tft.printf("AVG %u%% SCORE %u",rfEnvironmentState.averageOccupancy(),rfEnvironmentState.overallScore());
    } else if(appState.appMode==APP_MODE_ENV_HEATMAP){
        if(layout)drawModernHeader("TRAFFIC HEATMAP",accent);const uint16_t colors[6]={ST77XX_BLACK,ST77XX_DARKGRAY,SPECTRUM_LOW,SPECTRUM_MID,SPECTRUM_HIGH,SPECTRUM_CRITICAL};
        const int cols=rfEnvironmentState.historyCount;for(int x=0;x<cols;x++){uint8_t bi=(rfEnvironmentState.historyHead+RF_ENV_HISTORY_BUCKETS-cols+x)%RF_ENV_HISTORY_BUCKETS;for(int row=0;row<21;row++){int a=rfEnvironmentState.config.minChannel+row*(rfEnvironmentState.config.maxChannel-rfEnvironmentState.config.minChannel+1)/21,b=rfEnvironmentState.config.minChannel+(row+1)*(rfEnvironmentState.config.maxChannel-rfEnvironmentState.config.minChannel+1)/21;uint16_t sum=0,n=0;for(int ch=a;ch<b;ch++){sum+=rfEnvironmentState.history[bi][ch];n++;}uint8_t v=n?sum/n:0;tft.fillRect(25+x*4,18+(20-row)*4,4,4,colors[v?min(5,1+v/20):0]);}}
        if(layout){tft.setCursor(2,18);tft.setTextColor(ST77XX_GRAY,ST77XX_BLACK);tft.print("125");tft.setCursor(8,94);tft.print("0");}
    } else if(appState.appMode==APP_MODE_ENV_BURSTS){
        if(layout)drawModernHeader("BURST MONITOR",SPECTRUM_HIGH);clearLine(18);tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);tft.printf("EVENTS %u",rfEnvironmentState.eventCount);
        for(int y: {34,49,64,80})clearLine(y);if(rfEnvironmentState.eventCount){uint8_t idx=(rfEnvironmentState.eventHead+RF_ENV_BURST_EVENTS-1-envEventScroll)%RF_ENV_BURST_EVENTS;const RfBurstEvent&e=rfEnvironmentState.events[idx];tft.setCursor(4,34);tft.printf("#%lu  CH%u  %u MHz",(unsigned long)e.id,e.channel,e.frequencyMHz);tft.setCursor(4,49);tft.printf("PEAK %u%% BASE %u%%",e.peak,e.baseline);tft.setCursor(4,64);tft.printf("DELTA +%u  DUR %lums",e.delta,(unsigned long)e.durationMs);tft.setCursor(4,80);tft.print(e.severity==RF_BURST_HIGH?"HIGH":e.severity==RF_BURST_MEDIUM?"MEDIUM":"LOW");}
    } else if(appState.appMode==APP_MODE_ENV_COMPARE){
        if(layout)drawModernHeader("CHANNEL COMPARE",accent);for(int i=0;i<rfEnvironmentState.config.compareCount;i++){uint8_t ch=rfEnvironmentState.config.compareChannels[i];const RfChannelStats&s=rfEnvironmentState.channels[ch];int y=20+i*20;clearLine(y);tft.setCursor(3,y);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);tft.printf("CH%3u %3u%% %-4s",ch,s.movingAverage,rfEnvironmentState.scoreLabel(s.score));tft.fillRect(3,y+9,100,5,ST77XX_BLACK);tft.fillRect(3,y+9,s.movingAverage,5,getSignalColor(s.movingAverage));}
    } else if(appState.appMode==APP_MODE_ENV_STATUS){
        if(layout)drawModernHeader("RF STATUS",accent);uint8_t score=rfEnvironmentState.overallScore(),top[1];rfEnvironmentState.topChannels(top,1);uint16_t bursts=0;for(int i=0;i<TOTAL_CHANNELS;i++)bursts+=rfEnvironmentState.channels[i].burstCount;for(int y:{22,42,56,70,84})clearLine(y);tft.setCursor(5,22);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);tft.printf("SCORE %u/100  %s",score,rfEnvironmentState.scoreLabel(score));tft.setCursor(5,42);tft.printf("OCC     %u%%",rfEnvironmentState.averageOccupancy());tft.setCursor(5,56);tft.printf("BURSTS  %u",bursts);tft.setCursor(5,70);tft.printf("PEAK CH %u",top[0]);tft.setCursor(5,84);tft.print("RELATIVE ACTIVITY ONLY");
    } else if(appState.appMode==APP_MODE_ENV_BEFORE_AFTER){
        if(layout)drawModernHeader("BEFORE / AFTER",accent);for(int y:{18,35,55,72})clearLine(y);const auto&a=rfEnvironmentState.before;const auto&b=rfEnvironmentState.after;tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);if(!a.valid)tft.print("R: CAPTURE BEFORE");else if(!b.valid)tft.print("R: CAPTURE AFTER");else{tft.printf("AVG %u->%u %+d  PEAK %u->%u",a.average,b.average,(int)b.average-a.average,a.peak,b.peak);tft.setCursor(4,35);tft.printf("BURSTS %u->%u SCORE %u->%u",a.burstCount,b.burstCount,a.score,b.score);tft.setCursor(4,55);tft.printf("CH%u %u%% -> %u%%",envBandChannel,a.channelOccupancy[envBandChannel],b.channelOccupancy[envBandChannel]);tft.setCursor(4,72);tft.printf("DELTA %+d%%",(int)b.channelOccupancy[envBandChannel]-a.channelOccupancy[envBandChannel]);}}
    else if(appState.appMode==APP_MODE_ENV_BAND_INFO){
        if(layout)drawModernHeader("POSSIBLE OVERLAP",accent);for(int y:{18,35,50,64,78})clearLine(y);uint16_t f=RfEnvironmentMath::frequencyMHz(envBandChannel);tft.setCursor(4,18);tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);tft.printf("CH%u  %u MHz",envBandChannel,f);tft.setCursor(4,35);tft.print("POSSIBLE / BAND REGION:");int8_t wifi=RfEnvironmentMath::wifiChannelForMHz(f);tft.setCursor(4,50);if(wifi>0)tft.printf("WiFi CH%d overlap",wifi);else tft.print("WiFi: outside centers");tft.setCursor(4,64);tft.print(f>=2402&&f<=2480?"BLE / BT data region":"BLE / BT: outside");tft.setCursor(4,78);int8_t z=RfEnvironmentMath::zigbeeChannelForMHz(f);if(z>0)tft.printf("Zigbee CH%d center",z);else if(f>=2405&&f<=2480)tft.print("Zigbee band overlap");else tft.print("Zigbee: outside");
    } else {if(layout){drawModernHeader("AUTHORIZED RF PROBE",SPECTRUM_HIGH);tft.fillRoundRect(3,16,154,87,5,SPECTRUM_CARD_BG);tft.drawRoundRect(3,16,154,87,5,SPECTRUM_HIGH);tft.fillRoundRect(46,18,68,11,3,DISPLAY_ACTIVE_BG);tft.setCursor(50,20);tft.setTextColor(SPECTRUM_HIGH,DISPLAY_ACTIVE_BG);tft.print("LAB USE ONLY");}
#if RF_LAB_TX_ENABLED
        const auto&c=rfEnvironmentState.config;const char* labels[5]={"CHANNEL","INTERVAL","PACKETS","DURATION",rfAuthorizedProbe.isRunning()?"STOP PROBE":"START PROBE"};char values[4][12];snprintf(values[0],12,"CH %u",c.probeChannel);snprintf(values[1],12,"%u ms",c.probeIntervalMs);snprintf(values[2],12,"%u",c.probePacketCount);snprintf(values[3],12,"%u sec",c.probeMaxDurationSeconds);for(int i=0;i<5;i++){int y=31+i*13;bool sel=probeSelection==i;uint16_t rowBg=sel?SPECTRUM_HEADER_BG:SPECTRUM_CARD_BG;tft.fillRoundRect(8,y,144,11,3,rowBg);if(sel)tft.fillRect(8,y,3,11,SPECTRUM_ACCENT);tft.setCursor(14,y+2);tft.setTextColor(sel?ST77XX_WHITE:ST77XX_GRAY,rowBg);tft.print(labels[i]);if(i<4){tft.setCursor(104,y+2);tft.setTextColor(sel?SPECTRUM_ACCENT:ST77XX_WHITE,rowBg);tft.print(values[i]);}else{tft.setCursor(140,y+2);tft.setTextColor(rfAuthorizedProbe.isRunning()?SPECTRUM_CRITICAL:SPECTRUM_LOW,rowBg);tft.print(">");}}tft.fillRect(8,97,144,4,SPECTRUM_BORDER);int progress=c.probePacketCount?min<int>(144,(rfAuthorizedProbe.packetsSent()*144UL)/c.probePacketCount):0;if(progress)tft.fillRect(8,97,progress,4,rfAuthorizedProbe.isRunning()?SPECTRUM_HIGH:SPECTRUM_LOW);
#else
        tft.fillRoundRect(15,42,130,38,5,SPECTRUM_HEADER_BG);tft.setCursor(43,50);tft.setTextColor(SPECTRUM_LOW,SPECTRUM_HEADER_BG);tft.print("RX ONLY BUILD");tft.setCursor(27,65);tft.setTextColor(ST77XX_GRAY,SPECTRUM_HEADER_BG);tft.print("TX code not compiled");
#endif
    }
    if(layout)drawModernFooter(appState.appMode==APP_MODE_ENV_PROBE?"U/D SEL":"U/D VIEW",appState.appMode==APP_MODE_ENV_PROBE?"R ACTION":(rfEnvironmentState.running?"R STOP":"R START"),"B BACK");
    else if(!envRunningStatusValid||previousEnvRunning!=rfEnvironmentState.running)drawFooterChip(56,49,rfEnvironmentState.running?"R STOP":"R START");
    previousEnvRunning=rfEnvironmentState.running;envRunningStatusValid=true;
}

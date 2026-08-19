#include "ui/DisplayManager.h"
#include "drivers/ButtonManager.h"
#include "drivers/RadioManager.h"
#include "ui/MenuCatalog.h"

void DisplayManager::updateUI() {
    const int currentMode = static_cast<int>(appState.appMode);
    if (renderedMode != currentMode) {
        // A page transition is the only time the complete framebuffer area is
        // cleared. Updates within a page use the dirty regions below.
        tft.fillScreen(ST77XX_BLACK);
        renderedMode = currentMode;
        resetDynamicCaches();
        needRedraw = true;
        menuNeedsPartialRedraw = false;
    }

    switch (appState.appMode) {
        case APP_MODE_MENU:
            if (menuNeedsPartialRedraw) {
                redrawMenuItems(prevMenuSelection, menuSelection);
                menuNeedsPartialRedraw = false;
            } else if (needRedraw) {
                renderMainMenu();
                needRedraw = false;
                menuNeedsPartialRedraw = false;
            }
            break;
        case APP_MODE_JAMMER:
            if (needRedraw || lastJammerRenderMs == 0 ||
                millis() - lastJammerRenderMs >= 100) {
                renderJammerScreen();
                lastJammerRenderMs = millis();
                needRedraw = false;
            }
            break;
        case APP_MODE_ANALYZER_SPECTRUM:
            renderSpectrumAnalyzer();
            break;
        case APP_MODE_WATERFALL:
            renderWaterfallScreen();
            break;
        case APP_MODE_ANALYZER_CHANNEL:
            renderChannelInspector();   // per-frame dynamic update (static part redrawn internally)
            break;
        case APP_MODE_SURVEY:
            renderSurveyScreen();
            break;
        case APP_MODE_EVENTS:
            renderEventsScreen();
            break;
        case APP_MODE_LOGGING:
            if (needRedraw) renderLoggingScreen();
            break;
        case APP_MODE_RADIO_DIAG:
            if (needRedraw) renderRadioDiagScreen();
            break;
        case APP_MODE_PROFILES:
            if (needRedraw) renderProfilesScreen();
            break;
        case APP_MODE_SETTINGS:
            if (needRedraw) {
                renderSettingsScreen();
                needRedraw = false;
            }
            break;
        case APP_MODE_STATUS:
            if (needRedraw || lastStatusRenderMs == 0 ||
                millis() - lastStatusRenderMs >= 1000) {
                renderStatusScreen();
                needRedraw = false;
            }
            break;
        case APP_MODE_POWER:
            if (needRedraw) {
                renderPowerScreen();
                needRedraw = false;
            }
            break;
        case APP_MODE_REBOOT:
            renderRebootScreen();   // di-render tiap frame agar animasi titik hidup
            break;
        case APP_MODE_SHUTDOWN:
            renderShutdownScreen();
            break;
    }
}

// =============================================================================
// INPUT NAVIGATION
// =============================================================================
void DisplayManager::processInput() {
    // -------------------------------------------------------------------------
    // CONDITION 1: MAIN MENU
    // -------------------------------------------------------------------------
    if (appState.appMode == APP_MODE_MENU) {
        if (buttonManager.isLongPressed(BTN_UP) || buttonManager.isLongPressed(BTN_DOWN)) {
            menuPage = (menuPage + 1) % MenuCatalog::PAGE_COUNT;
            needRedraw = true;
            menuNeedsPartialRedraw = false;
        } else if (buttonManager.isPressed(BTN_UP)) {
            prevMenuSelection = menuSelection;
            menuSelection = (menuSelection - 1 + MenuCatalog::ITEMS_PER_PAGE) %
                            MenuCatalog::ITEMS_PER_PAGE;
            menuNeedsPartialRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            prevMenuSelection = menuSelection;
            menuSelection = (menuSelection + 1) % MenuCatalog::ITEMS_PER_PAGE;
            menuNeedsPartialRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            menuPage = (menuPage + 1) % MenuCatalog::PAGE_COUNT;
            needRedraw = true;
            menuNeedsPartialRedraw = false;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            const MenuFeature& feature =
                MenuCatalog::featureAt(menuPage, menuSelection);
            if (feature.openFlags & MENU_OPEN_STOP_RADIOS) {
                radioManager.stopAll();
            }
            if (feature.openFlags & MENU_OPEN_RESET_PEAKS) {
                appState.resetPeaks();
            }
            if (feature.openFlags & MENU_OPEN_RESET_INSPECTOR) {
                appState.inspectedPeak = 0;
            }
            appState.appMode = feature.mode;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 2: JAMMER MODE
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_JAMMER) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleJammerTarget(-1);
            if (appState.jamming) {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleJammerTarget(1);
            if (appState.jamming) {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            if (appState.jamming) {
                radioManager.stopJammer();
            } else {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopJammer();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 3: SPECTRUM ANALYZER
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_ANALYZER_SPECTRUM) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleAnalyzerBand(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleAnalyzerRadioMode(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.resetPeaks();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_WATERFALL) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleAnalyzerBand(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleAnalyzerRadioMode(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            memset(appState.waterfall, 0, sizeof(appState.waterfall));
            appState.waterfallHead = 0;
            appState.waterfallCount = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_SURVEY) {
        if (buttonManager.isPressed(BTN_UP) || buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleAnalyzerBand(1);
            appState.resetSurvey();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.resetSurvey();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_EVENTS) {
        if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.clearEvents();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_LOGGING) {
        if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.loggingEnabled = !appState.loggingEnabled;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_RADIO_DIAG) {
        if (buttonManager.isPressed(BTN_RIGHT)) {
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    else if (appState.appMode == APP_MODE_PROFILES) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleScanProfile(-1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleScanProfile(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT) &&
                   appState.scanProfile == SCAN_PROFILE_CUSTOM) {
            appState.cycleCustomSampleCount();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 4: CHANNEL INSPECTOR
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_ANALYZER_CHANNEL) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.inspectedChannel = constrain(appState.inspectedChannel + 1, MIN_CHANNEL, MAX_CHANNEL);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.inspectedChannel = constrain(appState.inspectedChannel - 1, MIN_CHANNEL, MAX_CHANNEL);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.inspectedChannel = (appState.inspectedChannel + 10) % (MAX_CHANNEL + 1);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 5: RF SETTINGS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_SETTINGS) {
        if (buttonManager.isPressed(BTN_UP)) {
            settingsSelection = (settingsSelection + 2) % 3;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            settingsSelection = (settingsSelection + 1) % 3;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            if (settingsSelection == 0) {
                appState.cyclePowerLevel(1);
                radioManager.updatePALevel(appState.powerLevel);
            } else if (settingsSelection == 1) {
                appState.cycleDwellTime(1);
            } else {
                appState.cycleDisplayTheme(1);
                // Force one clean page rebuild so no pixels from the previous
                // palette remain. Normal updates stay partial afterwards.
                renderedMode = -1;
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 6: STATUS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_STATUS) {
        if (buttonManager.isPressed(BTN_UP)) {
            statusPage = (statusPage + 2) % 3;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            statusPage = (statusPage + 1) % 3;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            lastStatusRenderMs = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 7: POWER OPTIONS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_POWER) {
        if (buttonManager.isPressed(BTN_UP) || buttonManager.isPressed(BTN_DOWN)) {
            powerSelection = (powerSelection + 1) % 2;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            radioManager.stopAll();
            appState.appMode = powerSelection == 0 ? APP_MODE_REBOOT : APP_MODE_SHUTDOWN;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
}

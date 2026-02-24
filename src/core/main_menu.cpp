#include "main_menu.h"
#include "../modules/bjs_interpreter/interpreter.h"
#include "display.h"
#include "main_menu.h"
#include "utils.h"
#include <LittleFS.h>
#include <globals.h>
#include <vector>
class DynamicJSApp : public MenuItemInterface {
public:
    String filePath;

    DynamicJSApp(String name, String path) : MenuItemInterface(name) { filePath = path; }

    void optionsMenu() override { run_bjs_script_headless(LittleFS, filePath); }

    void drawIcon(float scale = 1) override {
        tft.setTextSize(2 * scale);
        tft.setTextColor(TFT_CYAN);
        tft.drawCentreString(getName(), tftWidth / 2, tftHeight / 2 - 10, 1);

        tft.setTextSize(1 * scale);
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("(JS App)", tftWidth / 2, tftHeight / 2 + 15, 1);
    }

    bool hasTheme() override { return false; }

    String themePath() override { return ""; }
};

static std::vector<DynamicJSApp *> dynamicHomeApps;

void loadDynamicApps() {
    for (auto app : dynamicHomeApps) { delete app; }
    dynamicHomeApps.clear();

    if (!LittleFS.exists("/apps")) {
        LittleFS.mkdir("/apps");
        return;
    }

    File root = LittleFS.open("/apps");
    if (!root || !root.isDirectory()) return;

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;

        if (!isDir && (fullPath.endsWith(".js") || fullPath.endsWith(".JS"))) {
            String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
            nameOnly = nameOnly.substring(0, nameOnly.lastIndexOf("."));

            DynamicJSApp *newApp = new DynamicJSApp(nameOnly, fullPath);
            dynamicHomeApps.push_back(newApp);
        }
    }
    root.close();
}
// --------------------------------
MainMenu::MainMenu() {
    _menuItems = {
        &wifiMenu,
        &bleMenu,
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &rfMenu,
        &rfidMenu,
        &irMenu,
#if defined(FM_SI4713) && !defined(LITE_VERSION)
        &fmMenu,
#endif
        &fileMenu,
        &gpsMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
#if !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
#endif
        &loraMenu,
#endif
        &othersMenu,
        &clockMenu,
#if !defined(LITE_VERSION)
        &connectMenu,
#endif
        &configMenu,
    };

    _totalItems = _menuItems.size();
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    std::vector<String> l = bruceConfig.disabledMenus;

    // 1. Load all standard firmware apps
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) { // If menu item is not disabled
            options.push_back(
                {// selected lambda
                 _menuItems[i]->getName(),
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false,                                  // selected = false
                 [](void *menuItem, bool shouldRender) { // render lambda
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 _menuItems[i]
                }
            );
        }
    }

    loadDynamicApps();

    for (auto app : dynamicHomeApps) {
        String itemName = app->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) {
            options.push_back(
                {app->getName(),
                 [app]() { app->optionsMenu(); },
                 false,
                 [](void *menuItem, bool shouldRender) {
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 app}
            );
        }
    }
    // ----------------------------------------

    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
};

/*********************************************************************
**  Function: hideAppsMenu
**  Menu to Hide or show menus
**********************************************************************/

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
RESTART: // using gotos to avoid stackoverflow after many choices
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back({label, [this, label]() { bruceConfig.addDisabledMenu(label); }, enabled});
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    loopOptions(options);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}

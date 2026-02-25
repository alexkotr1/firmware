#include "MenuItemInterface.h"
#include <FS.h>

class DynamicJSApp : public MenuItemInterface {
public:
    String filePath;
    FS *fs; // <--- Store the filesystem pointer here

    // Require the filesystem in the constructor
    DynamicJSApp(String name, String path, FS *fileSystem);

    void optionsMenu() override;
    void drawIcon(float scale = 1) override;
    bool hasTheme() override;
    String themePath() override;
};

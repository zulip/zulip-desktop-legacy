#include "Setup.h"

#include "Config.h"

#ifdef HAVE_SPARKLE
    #import <Sparkle/SUUpdater.h>
#endif

void macMain() {
    // Initialize Sparkle
    [[SUUpdater sharedUpdater] setDelegate: NSApp];
}

#include "Setup.h"

#include "Config.h"

#ifdef HAVE_SPARKLE
    #import <Sparkle/SUUpdater.h>
#endif

void macMain() {
#ifdef HAVE_SPARKLE
    // Initialize Sparkle
    [[SUUpdater sharedUpdater] setDelegate: NSApp];
#endif
}

void checkForSparkleUpdate() {
#ifdef HAVE_SPARKLE
  [[SUUpdater sharedUpdater] checkForUpdates: NSApp];
#endif
}

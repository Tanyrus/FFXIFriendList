// PluginVersion.h

#ifndef PLUGIN_VERSION_H
#define PLUGIN_VERSION_H

namespace Plugin {

// Current plugin version (as string for display)
constexpr const char* PLUGIN_VERSION_STRING = "0.9.5";

// Current plugin version (as double for Ashita API)
// Format: major.minor -> 0.90 for version 0.9.0
constexpr double PLUGIN_VERSION = 0.95;

} // namespace Plugin

#endif // PLUGIN_VERSION_H


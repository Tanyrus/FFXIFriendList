#include "App/UseCases/PreferencesUseCases.h"
#include "Protocol/JsonUtils.h"
#include "Protocol/ResponseDecoder.h"
#include <algorithm>
#include <sstream>
#include <thread>

namespace XIFriendList {
namespace App {
namespace UseCases {

PreferencesUseCase::PreferencesUseCase(INetClient& netClient,
                                      IClock& clock,
                                      ILogger& logger,
                                      IPreferencesStore* preferencesStore)
    : netClient_(netClient)
    , clock_(clock)
    , logger_(logger)
    , preferencesStore_(preferencesStore)
    , loaded_(false)
{
}

XIFriendList::Core::Preferences PreferencesUseCase::getPreferences() const {
    return mergePreferences();
}

XIFriendList::Core::Preferences PreferencesUseCase::mergePreferences() const {
    XIFriendList::Core::Preferences merged;
    
    merged.useServerNotes = serverPreferences_.useServerNotes;
    merged.shareFriendsAcrossAlts = serverPreferences_.shareFriendsAcrossAlts;
    
    merged.mainFriendView = localPreferences_.mainFriendView;
    merged.quickOnlineFriendView = localPreferences_.quickOnlineFriendView;
    
    merged.debugMode = localPreferences_.debugMode;
    merged.overwriteNotesOnUpload = localPreferences_.overwriteNotesOnUpload;
    merged.overwriteNotesOnDownload = localPreferences_.overwriteNotesOnDownload;
    merged.shareJobWhenAnonymous = localPreferences_.shareJobWhenAnonymous;
    merged.showOnlineStatus = localPreferences_.showOnlineStatus;
    merged.shareLocation = localPreferences_.shareLocation;
    merged.notificationDuration = localPreferences_.notificationDuration;
    merged.customCloseKeyCode = localPreferences_.customCloseKeyCode;
    merged.controllerCloseButton = localPreferences_.controllerCloseButton;
    merged.windowsLocked = localPreferences_.windowsLocked;
    merged.notificationSoundsEnabled = localPreferences_.notificationSoundsEnabled;
    merged.soundOnFriendOnline = localPreferences_.soundOnFriendOnline;
    merged.soundOnFriendRequest = localPreferences_.soundOnFriendRequest;
    merged.notificationSoundVolume = localPreferences_.notificationSoundVolume;
    merged.notificationPositionX = localPreferences_.notificationPositionX;
    merged.notificationPositionY = localPreferences_.notificationPositionY;
    
    return merged;
}

PreferencesResult PreferencesUseCase::updateServerPreference(const std::string& field, bool value,
                                                             const std::string& apiKey,
                                                             const std::string& characterName) {
    std::string valueStr = value ? "true" : "false";
    std::string apiKeyStatus = apiKey.empty() ? "empty" : "present";
    std::string charNameStatus = characterName.empty() ? "empty" : characterName;
    logger_.debug("[pref] updateServerPreference: field=" + field + 
                 ", value=" + valueStr +
                 ", apiKey=" + apiKeyStatus +
                 ", characterName=" + charNameStatus);
    
    if (field == "useServerNotes") {
        serverPreferences_.useServerNotes = value;
    } else if (field == "shareFriendsAcrossAlts") {
        serverPreferences_.shareFriendsAcrossAlts = value;
    } else {
        logger_.warning("[pref] Unknown field: " + field);
        return PreferencesResult(false, "Unknown server preference field: " + field);
    }
    
    logger_.debug("[pref] Saving preferences after updating " + field);
    savePreferences(apiKey, characterName);
    return PreferencesResult(true);
}

PreferencesResult PreferencesUseCase::updateServerPreference(const std::string& field, const std::string& value,
                                                             const std::string& apiKey,
                                                             const std::string& characterName) {
    (void)value;
    (void)apiKey;
    (void)characterName;
    return PreferencesResult(false, "Unknown server preference field: " + field);
}

PreferencesResult PreferencesUseCase::updateLocalPreference(const std::string& field, bool value,
                                                             const std::string& apiKey,
                                                             const std::string& characterName) {
    if (field == "debugMode") {
        localPreferences_.debugMode = value;
    } else if (field == "overwriteNotesOnUpload") {
        localPreferences_.overwriteNotesOnUpload = value;
    } else if (field == "overwriteNotesOnDownload") {
        localPreferences_.overwriteNotesOnDownload = value;
    } else if (field == "shareJobWhenAnonymous") {
        localPreferences_.shareJobWhenAnonymous = value;
    } else if (field == "showOnlineStatus") {
        localPreferences_.showOnlineStatus = value;
    } else if (field == "shareLocation") {
        localPreferences_.shareLocation = value;
    } else if (field == "windowsLocked") {
        localPreferences_.windowsLocked = value;
    } else if (field == "notificationSoundsEnabled") {
        localPreferences_.notificationSoundsEnabled = value;
    } else if (field == "soundOnFriendOnline") {
        localPreferences_.soundOnFriendOnline = value;
    } else if (field == "soundOnFriendRequest") {
        localPreferences_.soundOnFriendRequest = value;
    } else if (field == "mainFriendView.showJob") {
        localPreferences_.mainFriendView.showJob = value;
    } else if (field == "mainFriendView.showZone") {
        localPreferences_.mainFriendView.showZone = value;
    } else if (field == "mainFriendView.showNationRank") {
        localPreferences_.mainFriendView.showNationRank = value;
    } else if (field == "mainFriendView.showLastSeen") {
        localPreferences_.mainFriendView.showLastSeen = value;
    } else if (field == "quickOnlineFriendView.showJob") {
        localPreferences_.quickOnlineFriendView.showJob = value;
    } else if (field == "quickOnlineFriendView.showZone") {
        localPreferences_.quickOnlineFriendView.showZone = value;
    } else if (field == "quickOnlineFriendView.showNationRank") {
        localPreferences_.quickOnlineFriendView.showNationRank = value;
    } else if (field == "quickOnlineFriendView.showLastSeen") {
        localPreferences_.quickOnlineFriendView.showLastSeen = value;
    } else {
        return PreferencesResult(false, "Unknown local preference field: " + field);
    }
    
    savePreferences(apiKey, characterName);
    return PreferencesResult(true);
}

PreferencesResult PreferencesUseCase::updateLocalPreference(const std::string& field, float value,
                                                             const std::string& apiKey,
                                                             const std::string& characterName) {
    if (field == "notificationDuration") {
        localPreferences_.notificationDuration = value;
    } else if (field == "customCloseKeyCode") {
        localPreferences_.customCloseKeyCode = static_cast<int>(value);
    } else if (field == "controllerCloseButton") {
        localPreferences_.controllerCloseButton = static_cast<int>(value);
    } else if (field == "notificationSoundVolume") {
        localPreferences_.notificationSoundVolume = value;
    } else if (field == "notificationPositionX") {
        localPreferences_.notificationPositionX = value;
    } else if (field == "notificationPositionY") {
        localPreferences_.notificationPositionY = value;
    } else {
        return PreferencesResult(false, "Unknown local preference field: " + field);
    }
    
    savePreferences(apiKey, characterName);
    return PreferencesResult(true);
}

PreferencesResult PreferencesUseCase::updateServerPreferences(const XIFriendList::Core::Preferences& prefs,
                                                              const std::string& apiKey,
                                                              const std::string& characterName) {
    serverPreferences_.useServerNotes = prefs.useServerNotes;
    serverPreferences_.shareFriendsAcrossAlts = prefs.shareFriendsAcrossAlts;
    
    savePreferences(apiKey, characterName);
    return PreferencesResult(true);
}

PreferencesResult PreferencesUseCase::updateLocalPreferences(const XIFriendList::Core::Preferences& prefs) {
    localPreferences_.debugMode = prefs.debugMode;
    localPreferences_.overwriteNotesOnUpload = prefs.overwriteNotesOnUpload;
    localPreferences_.overwriteNotesOnDownload = prefs.overwriteNotesOnDownload;
    localPreferences_.shareJobWhenAnonymous = prefs.shareJobWhenAnonymous;
    localPreferences_.showOnlineStatus = prefs.showOnlineStatus;
    localPreferences_.shareLocation = prefs.shareLocation;
    localPreferences_.notificationDuration = prefs.notificationDuration;
    localPreferences_.customCloseKeyCode = prefs.customCloseKeyCode;
    localPreferences_.controllerCloseButton = prefs.controllerCloseButton;
    localPreferences_.windowsLocked = prefs.windowsLocked;
    localPreferences_.notificationSoundsEnabled = prefs.notificationSoundsEnabled;
    localPreferences_.soundOnFriendOnline = prefs.soundOnFriendOnline;
    localPreferences_.soundOnFriendRequest = prefs.soundOnFriendRequest;
    localPreferences_.notificationSoundVolume = prefs.notificationSoundVolume;
    localPreferences_.mainFriendView = prefs.mainFriendView;
    localPreferences_.quickOnlineFriendView = prefs.quickOnlineFriendView;
    localPreferences_.notificationPositionX = prefs.notificationPositionX;
    localPreferences_.notificationPositionY = prefs.notificationPositionY;
    
    savePreferences();
    return PreferencesResult(true);
}

PreferencesResult PreferencesUseCase::resetPreferences() {
    serverPreferences_ = XIFriendList::Core::Preferences();
    localPreferences_ = XIFriendList::Core::Preferences();
    
    savePreferences();
    return PreferencesResult(true);
}

void PreferencesUseCase::loadPreferences(const std::string& apiKey, 
                                         const std::string& characterName) {
    if (!apiKey.empty() && !characterName.empty() && netClient_.isAvailable()) {
        if (loadServerPreferencesFromServer(apiKey, characterName)) {
            logger_.debug("[pref] Loaded preferences from server");
        } else {
            logger_.debug("[pref] Failed to load from server, using local file");
            if (preferencesStore_) {
                serverPreferences_ = preferencesStore_->loadServerPreferences();
            }
        }
    } else {
        if (preferencesStore_) {
            serverPreferences_ = preferencesStore_->loadServerPreferences();
        }
    }
    
    if (preferencesStore_) {
        localPreferences_ = preferencesStore_->loadLocalPreferences();
    }
    
    loaded_ = true;
}

void PreferencesUseCase::savePreferences(const std::string& apiKey, 
                                        const std::string& characterName) {
    std::string apiKeyStatus = apiKey.empty() ? "empty" : "present";
    std::string charNameStatus = characterName.empty() ? "empty" : characterName;
    std::string netClientStatus = netClient_.isAvailable() ? "true" : "false";
    logger_.debug("[pref] savePreferences: apiKey=" + apiKeyStatus + 
                 ", characterName=" + charNameStatus +
                 ", netClientAvailable=" + netClientStatus);
    
    if (!apiKey.empty() && !characterName.empty() && netClient_.isAvailable()) {
        logger_.debug("[pref] Starting server sync");
        std::thread([this, apiKey, characterName]() {
            syncServerPreferencesToServer(serverPreferences_, apiKey, characterName);
        }).detach();
    } else {
        std::string apiKeyStatusSkip = apiKey.empty() ? "empty" : "present";
        std::string charNameStatusSkip = characterName.empty() ? "empty" : "present";
        std::string netClientStatusSkip = netClient_.isAvailable() ? "true" : "false";
        logger_.debug("[pref] Skipping server sync - apiKey=" + apiKeyStatusSkip + 
                       ", characterName=" + charNameStatusSkip +
                       ", netClientAvailable=" + netClientStatusSkip);
    }
    
    if (preferencesStore_) {
        preferencesStore_->saveServerPreferences(serverPreferences_);
        preferencesStore_->saveLocalPreferences(localPreferences_);
        logger_.debug("[pref] Saved preferences to local file");
    }
}

PreferencesResult PreferencesUseCase::syncFromServer(const std::string& apiKey, 
                                                     const std::string& characterName) {
    if (apiKey.empty() || characterName.empty()) {
        return PreferencesResult(false, "API key and character name required");
    }
    
    if (!netClient_.isAvailable()) {
        return PreferencesResult(false, "Network client not available");
    }
    
    if (loadServerPreferencesFromServer(apiKey, characterName)) {
        if (preferencesStore_) {
            preferencesStore_->saveServerPreferences(serverPreferences_);
        }
        return PreferencesResult(true);
    }
    
    return PreferencesResult(false, "Failed to sync preferences from server");
}

bool PreferencesUseCase::syncServerPreferencesToServer(const XIFriendList::Core::Preferences& prefs,
                                                       const std::string& apiKey,
                                                       const std::string& characterName) {
    try {
        logger_.debug("[pref] syncServerPreferencesToServer");
        
        std::vector<std::pair<std::string, std::string>> preferencesFields;
        preferencesFields.push_back({"useServerNotes", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.useServerNotes)});
        preferencesFields.push_back({"shareFriendsAcrossAlts", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.shareFriendsAcrossAlts)});
        preferencesFields.push_back(std::make_pair("mainFriendView.showJob", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.mainFriendView.showJob)));
        preferencesFields.push_back(std::make_pair("mainFriendView.showZone", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.mainFriendView.showZone)));
        preferencesFields.push_back(std::make_pair("mainFriendView.showNationRank", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.mainFriendView.showNationRank)));
        preferencesFields.push_back(std::make_pair("mainFriendView.showLastSeen", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.mainFriendView.showLastSeen)));
        preferencesFields.push_back(std::make_pair("quickOnlineFriendView.showJob", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showJob)));
        preferencesFields.push_back(std::make_pair("quickOnlineFriendView.showZone", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showZone)));
        preferencesFields.push_back(std::make_pair("quickOnlineFriendView.showNationRank", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showNationRank)));
        preferencesFields.push_back(std::make_pair("quickOnlineFriendView.showLastSeen", XIFriendList::Protocol::JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showLastSeen)));
        
        std::string preferencesJson = XIFriendList::Protocol::JsonUtils::encodeObject(preferencesFields);
        
        std::vector<std::pair<std::string, std::string>> requestFields;
        requestFields.push_back({"preferences", preferencesJson});
        std::string requestJson = XIFriendList::Protocol::JsonUtils::encodeObject(requestFields);
        
        std::string url = netClient_.getBaseUrl() + "/api/preferences";
        
        logger_.debug("[pref] Sending PATCH to " + url);
        logger_.debug("[pref] Request payload: " + requestJson);
        
        XIFriendList::App::HttpResponse response = netClient_.patch(url, apiKey, characterName, requestJson);
        
        if (response.isSuccess()) {
            logger_.info("[pref] Synced to server (statusCode=" + 
                        std::to_string(response.statusCode) + ")");
            return true;
        } else {
            logger_.error("[pref] Failed to sync to server: " + 
                         (response.error.empty() ? "HTTP " + std::to_string(response.statusCode) : response.error) +
                         ", response body: " + response.body);
            return false;
        }
    } catch (const std::exception& e) {
        logger_.error("[pref] Exception syncing to server: " + std::string(e.what()));
        return false;
    } catch (...) {
        logger_.error("[pref] Unknown exception syncing to server");
        return false;
    }
}

bool PreferencesUseCase::loadServerPreferencesFromServer(const std::string& apiKey,
                                                         const std::string& characterName) {
    try {
        std::string url = netClient_.getBaseUrl() + "/api/preferences";
        
        XIFriendList::App::HttpResponse response = netClient_.get(url, apiKey, characterName);
        
        if (!response.isSuccess()) {
            return false;
        }
        
        XIFriendList::Protocol::ResponseMessage responseMsg;
        XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
        
        if (decodeResult != XIFriendList::Protocol::DecodeResult::Success || 
            responseMsg.type != XIFriendList::Protocol::ResponseType::Preferences ||
            !responseMsg.success) {
            return false;
        }
        
        std::string decodedPayload = responseMsg.payload;
        if (!responseMsg.payload.empty() && responseMsg.payload[0] == '"') {
            XIFriendList::Protocol::JsonUtils::decodeString(responseMsg.payload, decodedPayload);
        }
        
        XIFriendList::Protocol::PreferencesResponsePayload payload;
        decodeResult = XIFriendList::Protocol::ResponseDecoder::decodePreferencesPayload(decodedPayload, payload);
        
        if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
            return false;
        }
        
        serverPreferences_.useServerNotes = payload.useServerNotes;
        serverPreferences_.shareFriendsAcrossAlts = payload.shareFriendsAcrossAlts;
        
        // Map old server protocol fields to new FriendViewSettings structure
        serverPreferences_.mainFriendView.showJob = payload.showJobColumn;
        serverPreferences_.mainFriendView.showZone = payload.showZoneColumn;
        serverPreferences_.mainFriendView.showNationRank = payload.showNationColumn || payload.showRankColumn;
        serverPreferences_.mainFriendView.showLastSeen = payload.showLastSeenColumn;
        
        serverPreferences_.quickOnlineFriendView.showJob = payload.quickOnlineShowJobColumn;
        serverPreferences_.quickOnlineFriendView.showZone = payload.quickOnlineShowZoneColumn;
        serverPreferences_.quickOnlineFriendView.showNationRank = payload.quickOnlineShowNationColumn || payload.quickOnlineShowRankColumn;
        serverPreferences_.quickOnlineFriendView.showLastSeen = payload.quickOnlineShowLastSeenColumn;
        
        return true;
    } catch (...) {
        logger_.error("[pref] Exception loading from server");
        return false;
    }
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList


#include "App/UseCases/NotesUseCases.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include "Protocol/JsonUtils.h"
#include "Core/NotesCore.h"
#include <functional>
#include <sstream>
#include <chrono>
#include <optional>
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace App {
namespace UseCases {

namespace {
    std::string normalizeFriendName(const std::string& name) {
        std::string normalized = name;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return normalized;
    }
} // namespace

SaveNoteResult SaveNoteUseCase::saveNote(const std::string& apiKey,
                                        const std::string& characterName,
                                        const std::string& friendName,
                                        const std::string& noteText,
                                        bool useServerNotes) {
    if (friendName.empty()) {
        return { false, "Friend name required", XIFriendList::Core::Note() };
    }
    
    if (noteText.length() > 8192) {
        return { false, "Note must be 8192 characters or less", XIFriendList::Core::Note() };
    }
    
    return saveNoteToLocal(friendName, noteText);
}

SaveNoteResult SaveNoteUseCase::saveNoteToServer(const std::string& apiKey,
                                                  const std::string& characterName,
                                                  const std::string& friendName,
                                                  const std::string& noteText) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", XIFriendList::Core::Note() };
    }
    
    logger_.info("[notes] Saving note to server for " + friendName);
    
    std::string url = netClient_.getBaseUrl() + "/api/notes/" + friendName;
    
    auto requestFunc = [this, url, apiKey, characterName, friendName, noteText]() {
        std::string payload = XIFriendList::Protocol::RequestEncoder::encodePutNote(friendName, noteText);
        return netClient_.post(url, apiKey, characterName, payload);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "PutNote");
    
    if (!response.isSuccess()) {
        std::string error;
        
        if (response.statusCode == 0) {
            error = response.error.empty() ? "Network error: failed to save note" : response.error;
            logger_.error("[notes] Network error: " + error);
            return { false, error, XIFriendList::Core::Note() };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success && !responseMsg.error.empty()) {
                error = "SaveNote failed: " + responseMsg.error;
            } else {
                error = "SaveNote failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[notes] " + error);
            return { false, error, XIFriendList::Core::Note() };
        }
        
        error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[notes] Failed: " + error);
        return { false, error, XIFriendList::Core::Note() };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode response");
        return { false, "Invalid response format", XIFriendList::Core::Note() };
    }
    
    if (!responseMsg.success) {
        std::string error = responseMsg.error.empty() ? "Failed to save note" : responseMsg.error;
        logger_.error("[notes] Server returned error: " + error);
        return { false, error, XIFriendList::Core::Note() };
    }
    
    if (responseMsg.type != XIFriendList::Protocol::ResponseType::Note && responseMsg.type != XIFriendList::Protocol::ResponseType::Success) {
        logger_.error("[notes] Unexpected response type");
        return { false, "Invalid response type", XIFriendList::Core::Note() };
    }
    
    std::string decodedPayload = responseMsg.payload;
    if (!decodedPayload.empty() && decodedPayload[0] == '"') {
        std::string temp;
        if (XIFriendList::Protocol::JsonUtils::decodeString(decodedPayload, temp)) {
            decodedPayload = temp;
        }
    }
    
    XIFriendList::Protocol::NoteResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeNotePayload(decodedPayload, payload);
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode note payload");
        return { false, "Invalid note format", XIFriendList::Core::Note() };
    }
    
    XIFriendList::Core::Note note = convertToCore(payload.note);
    
    logger_.info("[notes] Saved note for " + friendName + " to server");
    return { true, "", note };
}

SaveNoteResult SaveNoteUseCase::saveNoteToLocal(const std::string& friendName,
                                                const std::string& noteText) {
    logger_.info("[notes] Saving note to local storage for " + friendName);
    
    try {
        uint64_t updatedAt = clock_.nowMs();
        std::string normalized = normalizeFriendName(friendName);
        
        XIFriendList::Core::Note note(normalized, noteText, updatedAt);
        notesState_.notes[normalized] = note;
        notesState_.dirty = true;
        
        logger_.info("[notes] Saved note for " + friendName + " to local storage");
        return { true, "", note };
    } catch (const std::exception& e) {
        std::string error = "Failed to save note to local storage: " + std::string(e.what());
        logger_.error("[notes] " + error);
        return { false, error, XIFriendList::Core::Note() };
    }
}

XIFriendList::Core::Note SaveNoteUseCase::convertToCore(const XIFriendList::Protocol::NoteData& data) {
    XIFriendList::Core::Note note;
    note.friendName = data.friendName;
    note.note = data.note;
    note.updatedAt = data.updatedAt;
    return note;
}

XIFriendList::App::HttpResponse SaveNoteUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                               const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        if (response.statusCode >= 400 && response.statusCode < 500) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Retry " + std::to_string(attempts) + "/" + std::to_string(maxRetries_));
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}

DeleteNoteResult DeleteNoteUseCase::deleteNote(const std::string& apiKey,
                                              const std::string& characterName,
                                              const std::string& friendName,
                                              bool useServerNotes) {
    if (friendName.empty()) {
        return { false, "Friend name required" };
    }
    
    if (useServerNotes) {
        return deleteNoteFromServer(apiKey, characterName, friendName);
    } else {
        return deleteNoteFromLocal(friendName);
    }
}

DeleteNoteResult DeleteNoteUseCase::deleteNoteFromServer(const std::string& apiKey,
                                                         const std::string& characterName,
                                                         const std::string& friendName) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required" };
    }
    
    logger_.info("[notes] Deleting note from server for " + friendName);
    
    std::string url = netClient_.getBaseUrl() + "/api/notes/" + friendName;
    
    auto requestFunc = [this, url, apiKey, characterName, friendName]() {
        std::string payload = XIFriendList::Protocol::RequestEncoder::encodeDeleteNote(friendName);
        return netClient_.del(url, apiKey, characterName, payload);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "DeleteNote");
    
    if (!response.isSuccess()) {
        std::string error;
        
        if (response.statusCode == 0) {
            error = response.error.empty() ? "Network error: failed to delete note" : response.error;
            logger_.error("[notes] Network error: " + error);
            return { false, error };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                if (response.statusCode == 404) {
                    logger_.info("[notes] Note not found (already deleted)");
                    return { true, "" };
                }
                error = responseMsg.error.empty() ? "DeleteNote failed" : responseMsg.error;
            } else {
                error = "DeleteNote failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[notes] " + error);
            return { false, error };
        }
        
        error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[notes] Failed: " + error);
        return { false, error };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode response");
        return { false, "Invalid response format" };
    }
    
    if (!responseMsg.success) {
        std::string error = responseMsg.error.empty() ? "Failed to delete note" : responseMsg.error;
        logger_.error("[notes] Server returned error: " + error);
        return { false, error };
    }
    
    logger_.info("[notes] Deleted note for " + friendName + " from server");
    return { true, "" };
}

DeleteNoteResult DeleteNoteUseCase::deleteNoteFromLocal(const std::string& friendName) {
    logger_.info("[notes] Deleting note from local storage for " + friendName);
    
    try {
        std::string normalized = normalizeFriendName(friendName);
        
        auto it = notesState_.notes.find(normalized);
        if (it != notesState_.notes.end()) {
            notesState_.notes.erase(it);
            notesState_.dirty = true;
            logger_.info("[notes] Deleted note for " + friendName + " from local storage");
        } else {
            logger_.info("[notes] Note not found (already deleted)");
        }
        
        return { true, "" };
    } catch (const std::exception& e) {
        std::string error = "Failed to delete note from local storage: " + std::string(e.what());
        logger_.error("[notes] " + error);
        return { false, error };
    }
}

XIFriendList::App::HttpResponse DeleteNoteUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                 const std::string& operationName) {
    int attempts = 0;
    XIFriendList::App::HttpResponse response;
    while (attempts < maxRetries_) {
        response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        if (response.statusCode >= 400 && response.statusCode < 500) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Retry " + std::to_string(attempts) + "/" + std::to_string(maxRetries_));
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return response;
}

GetNotesResult GetNotesUseCase::getNotes(const std::string& apiKey,
                                        const std::string& characterName,
                                        bool useServerNotes) {
    if (useServerNotes) {
        return getNotesFromServer(apiKey, characterName);
    } else {
        return getNotesFromLocal();
    }
}

GetNotesResult GetNotesUseCase::getNote(const std::string& apiKey,
                                       const std::string& characterName,
                                       const std::string& friendName,
                                       bool useServerNotes) {
    if (friendName.empty()) {
        return { false, "Friend name required", {} };
    }
    
    if (useServerNotes) {
        return getNoteFromServer(apiKey, characterName, friendName);
    } else {
        return getNoteFromLocal(friendName);
    }
}

GetNotesResult GetNotesUseCase::getNotesFromServer(const std::string& apiKey,
                                                   const std::string& characterName) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", {} };
    }
    
    logger_.info("[notes] Getting notes from server");
    
    std::string url = netClient_.getBaseUrl() + "/api/notes";
    
    auto requestFunc = [this, url, apiKey, characterName]() {
        std::string payload = XIFriendList::Protocol::RequestEncoder::encodeGetNotes();
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetNotes");
    
    if (!response.isSuccess()) {
        std::string error;
        
        if (response.statusCode == 0) {
            error = response.error.empty() ? "Network error: failed to get notes" : response.error;
            logger_.error("[notes] Network error: " + error);
            return { false, error, {} };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success && !responseMsg.error.empty()) {
                error = "GetNotes failed: " + responseMsg.error;
            } else {
                error = "GetNotes failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[notes] " + error);
            return { false, error, {} };
        }
        
        error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[notes] Failed: " + error);
        return { false, error, {} };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode response");
        return { false, "Invalid response format", {} };
    }
    
    if (!responseMsg.success) {
        std::string error = responseMsg.error.empty() ? "Failed to get notes" : responseMsg.error;
        logger_.error("[notes] Server returned error: " + error);
        return { false, error, {} };
    }
    
    if (responseMsg.type != XIFriendList::Protocol::ResponseType::NotesList) {
        logger_.error("[notes] Unexpected response type");
        return { false, "Invalid response type", {} };
    }
    
    std::string decodedPayload = responseMsg.payload;
    if (!decodedPayload.empty() && decodedPayload[0] == '"') {
        std::string temp;
        if (XIFriendList::Protocol::JsonUtils::decodeString(decodedPayload, temp)) {
            decodedPayload = temp;
        }
    }
    
    XIFriendList::Protocol::NotesListResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeNotesListPayload(decodedPayload, payload);
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode notes list payload");
        return { false, "Invalid notes list format", {} };
    }
    
    std::map<std::string, XIFriendList::Core::Note> notes;
    for (const auto& noteData : payload.notes) {
        XIFriendList::Core::Note note = convertToCore(noteData);
        notes[note.friendName] = note;
    }
    
    logger_.info("[notes] Retrieved " + std::to_string(notes.size()) + " notes from server");
    return { true, "", notes };
}

GetNotesResult GetNotesUseCase::getNoteFromServer(const std::string& apiKey,
                                                  const std::string& characterName,
                                                  const std::string& friendName) {
    if (apiKey.empty() || characterName.empty() || friendName.empty()) {
        return { false, "API key, character name, and friend name required", {} };
    }
    
    logger_.info("[notes] Getting note from server for " + friendName);
    
    std::string url = netClient_.getBaseUrl() + "/api/notes/" + friendName;
    
    auto requestFunc = [this, url, apiKey, characterName, friendName]() {
        std::string payload = XIFriendList::Protocol::RequestEncoder::encodeGetNote(friendName);
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetNote");
    
    if (!response.isSuccess()) {
        std::string error;
        
        if (response.statusCode == 0) {
            error = response.error.empty() ? "Network error: failed to get note" : response.error;
            logger_.error("[notes] Network error: " + error);
            return { false, error, {} };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                if (response.statusCode == 404) {
                    return { true, "", {} };
                }
                error = responseMsg.error.empty() ? "GetNote failed" : responseMsg.error;
            } else {
                error = "GetNote failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[notes] " + error);
            return { false, error, {} };
        }
        
        error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[notes] Failed: " + error);
        return { false, error, {} };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode response");
        return { false, "Invalid response format", {} };
    }
    
    if (!responseMsg.success) {
        std::string error = responseMsg.error.empty() ? "Failed to get note" : responseMsg.error;
        logger_.error("[notes] Server returned error: " + error);
        return { false, error, {} };
    }
    
    if (responseMsg.type != XIFriendList::Protocol::ResponseType::Note) {
        logger_.error("[notes] Unexpected response type");
        return { false, "Invalid response type", {} };
    }
    
    std::string decodedPayload = responseMsg.payload;
    if (!decodedPayload.empty() && decodedPayload[0] == '"') {
        std::string temp;
        if (XIFriendList::Protocol::JsonUtils::decodeString(decodedPayload, temp)) {
            decodedPayload = temp;
        }
    }
    
    XIFriendList::Protocol::NoteResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeNotePayload(decodedPayload, payload);
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[notes] Failed to decode note payload");
        return { false, "Invalid note format", {} };
    }
    
    XIFriendList::Core::Note note = convertToCore(payload.note);
    std::map<std::string, XIFriendList::Core::Note> notes;
    notes[note.friendName] = note;
    
    logger_.info("[notes] Retrieved note for " + friendName + " from server");
    return { true, "", notes };
}

GetNotesResult GetNotesUseCase::getNotesFromLocal() {
    logger_.info("[notes] Getting notes from local storage");
    
    try {
        std::map<std::string, XIFriendList::Core::Note> notes = notesState_.notes;
        logger_.info("[notes] Retrieved " + std::to_string(notes.size()) + " notes from local storage");
        return { true, "", notes };
    } catch (const std::exception& e) {
        std::string error = "Failed to get notes from local storage: " + std::string(e.what());
        logger_.error("[notes] " + error);
        return { false, error, {} };
    }
}

GetNotesResult GetNotesUseCase::getNoteFromLocal(const std::string& friendName) {
    logger_.info("[notes] Getting note from local storage for " + friendName);
    
    try {
        std::string normalized = normalizeFriendName(friendName);
        auto it = notesState_.notes.find(normalized);
        
        if (it != notesState_.notes.end()) {
            std::map<std::string, XIFriendList::Core::Note> notes;
            notes[it->second.friendName] = it->second;
            logger_.info("[notes] Retrieved note for " + friendName + " from local storage");
            return { true, "", notes };
        } else {
            return { true, "", {} };
        }
    } catch (const std::exception& e) {
        std::string error = "Failed to get note from local storage: " + std::string(e.what());
        logger_.error("[notes] " + error);
        return { false, error, {} };
    }
}

XIFriendList::Core::Note GetNotesUseCase::convertToCore(const XIFriendList::Protocol::NoteData& data) {
    XIFriendList::Core::Note note;
    note.friendName = data.friendName;
    note.note = data.note;
    note.updatedAt = data.updatedAt;
    return note;
}

XIFriendList::App::HttpResponse GetNotesUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                               const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        if (response.statusCode >= 400 && response.statusCode < 500) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Retry " + std::to_string(attempts) + "/" + std::to_string(maxRetries_));
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList


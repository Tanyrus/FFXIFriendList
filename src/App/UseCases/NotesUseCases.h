
#ifndef APP_NOTES_USE_CASES_H
#define APP_NOTES_USE_CASES_H

#include "App/Interfaces/INetClient.h"
#include "App/State/NotesState.h"
#include "App/Interfaces/IClock.h"
#include "App/Interfaces/ILogger.h"
#include "Core/NotesCore.h"
#include "Core/ModelsCore.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include <string>
#include <map>
#include <optional>
#include <functional>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct SaveNoteResult {
    bool success;
    std::string error;
    XIFriendList::Core::Note note;
    
    SaveNoteResult() : success(false) {}
    SaveNoteResult(bool success, const std::string& error = "", const XIFriendList::Core::Note& note = XIFriendList::Core::Note())
        : success(success), error(error), note(note) {}
};

class SaveNoteUseCase {
public:
    SaveNoteUseCase(INetClient& netClient,
                   XIFriendList::App::State::NotesState& notesState,
                   IClock& clock,
                   ILogger& logger)
        : netClient_(netClient)
        , notesState_(notesState)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    SaveNoteResult saveNote(const std::string& apiKey,
                           const std::string& characterName,
                           const std::string& friendName,
                           const std::string& noteText,
                           bool useServerNotes);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    SaveNoteResult saveNoteToServer(const std::string& apiKey,
                                   const std::string& characterName,
                                   const std::string& friendName,
                                   const std::string& noteText);
    SaveNoteResult saveNoteToLocal(const std::string& friendName,
                                  const std::string& noteText);
    XIFriendList::Core::Note convertToCore(const XIFriendList::Protocol::NoteData& data);
    
    INetClient& netClient_;
    XIFriendList::App::State::NotesState& notesState_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct DeleteNoteResult {
    bool success;
    std::string error;
    
    DeleteNoteResult() : success(false) {}
    DeleteNoteResult(bool success, const std::string& error = "")
        : success(success), error(error) {}
};

class DeleteNoteUseCase {
public:
    DeleteNoteUseCase(INetClient& netClient,
                     XIFriendList::App::State::NotesState& notesState,
                     IClock& clock,
                     ILogger& logger)
        : netClient_(netClient)
        , notesState_(notesState)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    DeleteNoteResult deleteNote(const std::string& apiKey,
                               const std::string& characterName,
                               const std::string& friendName,
                               bool useServerNotes);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    DeleteNoteResult deleteNoteFromServer(const std::string& apiKey,
                                         const std::string& characterName,
                                         const std::string& friendName);
    DeleteNoteResult deleteNoteFromLocal(const std::string& friendName);
    
    INetClient& netClient_;
    XIFriendList::App::State::NotesState& notesState_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct GetNotesResult {
    bool success;
    std::string error;
    std::map<std::string, XIFriendList::Core::Note> notes;
    
    GetNotesResult() : success(false) {}
    GetNotesResult(bool success, const std::string& error = "", const std::map<std::string, XIFriendList::Core::Note>& notes = {})
        : success(success), error(error), notes(notes) {}
};

class GetNotesUseCase {
public:
    GetNotesUseCase(INetClient& netClient,
                   XIFriendList::App::State::NotesState& notesState,
                   IClock& clock,
                   ILogger& logger)
        : netClient_(netClient)
        , notesState_(notesState)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    GetNotesResult getNotes(const std::string& apiKey,
                           const std::string& characterName,
                           bool useServerNotes);
    GetNotesResult getNote(const std::string& apiKey,
                          const std::string& characterName,
                          const std::string& friendName,
                          bool useServerNotes);
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    GetNotesResult getNotesFromServer(const std::string& apiKey,
                                     const std::string& characterName);
    GetNotesResult getNoteFromServer(const std::string& apiKey,
                                    const std::string& characterName,
                                    const std::string& friendName);
    GetNotesResult getNotesFromLocal();
    GetNotesResult getNoteFromLocal(const std::string& friendName);
    XIFriendList::Core::Note convertToCore(const XIFriendList::Protocol::NoteData& data);
    
    INetClient& netClient_;
    XIFriendList::App::State::NotesState& notesState_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_NOTES_USE_CASES_H


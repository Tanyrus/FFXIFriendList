cmake_minimum_required(VERSION 3.15)

project(FFXIFriendList
    VERSION 0.9.6
    LANGUAGES CXX
)

# ==================================================
# Global C++ configuration
# ==================================================
# Ashita SDK requires C++20 for std::ranges support
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================================================
# MSVC Runtime Library Configuration
# ==================================================
# Use static runtime linking (/MT) so the DLL doesn't require
# Visual C++ Redistributable to be installed on target systems
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ==================================================
# Build options
# ==================================================
option(BUILD_TESTS "Build tests" ON)

# ==================================================
# Enforce 32-bit build (FFXI / Ashita requirement)
# ==================================================
# For Visual Studio generators: use -A Win32
# For Ninja: must run CMake from x86 MSVC toolchain (vcvars32.bat or x86 Native Tools)
# CMAKE_SIZEOF_VOID_P is set by CMake based on the compiler architecture, not the generator
if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (CMAKE_GENERATOR MATCHES "Ninja")
        message(FATAL_ERROR
            "This plugin must be built as 32-bit (x86).\n"
            "For Ninja builds, run CMake from the x86 MSVC toolchain:\n"
            "  vcvars32.bat\n"
            "  cmake -G Ninja -S . -B build\n"
            "Or use: cmake -G \"Ninja Multi-Config\" -T v143_xp ..."
        )
    else()
        message(FATAL_ERROR
            "This plugin must be built as 32-bit (Win32).\n"
            "Reconfigure with: cmake -A Win32 ..."
        )
    endif()
endif()

# ==================================================
# Output directories
# ==================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==================================================
# Ashita SDK configuration
# ==================================================
# Set the path to your Ashita v4 ADK include directory
# Example: set(ASHITA_SDK_DIR "C:/HorizonXI/HorizonXI/Game/plugins/sdk")
# If not set, will try common locations
if(NOT DEFINED ASHITA_SDK_DIR)
    # Try common locations
    if(EXISTS "C:/HorizonXI/HorizonXI/Game/plugins/sdk")
        set(ASHITA_SDK_DIR "C:/HorizonXI/HorizonXI/Game/plugins/sdk")
    elseif(EXISTS "C:/Users/Tanyrus/Games/HorizonXI/Game/plugins/sdk")
        set(ASHITA_SDK_DIR "C:/Users/Tanyrus/Games/HorizonXI/Game/plugins/sdk")
    elseif(EXISTS "C:/HorizonXI/Game/plugins/sdk")
        set(ASHITA_SDK_DIR "C:/HorizonXI/Game/plugins/sdk")
    elseif(EXISTS "C:/Ashita/plugins/sdk")
        set(ASHITA_SDK_DIR "C:/Ashita/plugins/sdk")
    else()
        message(WARNING
            "ASHITA_SDK_DIR not set and common locations not found.\n"
            "Please set ASHITA_SDK_DIR to your Ashita v4 ADK include directory:\n"
            "  cmake -DASHITA_SDK_DIR=C:/path/to/ashita/plugins/sdk ..."
        )
    endif()
endif()

if(EXISTS "${ASHITA_SDK_DIR}/Ashita.h")
    message(STATUS "Found Ashita SDK at: ${ASHITA_SDK_DIR}")
    # Verify imgui.h exists in SDK directory
    if(EXISTS "${ASHITA_SDK_DIR}/imgui.h")
        message(STATUS "Found imgui.h in SDK directory: ${ASHITA_SDK_DIR}/imgui.h")
    else()
        message(WARNING "imgui.h not found in SDK directory: ${ASHITA_SDK_DIR}/imgui.h")
    endif()
else()
    message(FATAL_ERROR
        "Ashita SDK not found at: ${ASHITA_SDK_DIR}\n"
        "Please set ASHITA_SDK_DIR to the correct path containing Ashita.h"
    )
endif()

# ==================================================
# Centralized build options (INTERFACE library)
# ==================================================
# This INTERFACE library centralizes all compile flags to eliminate duplication
# and ensure consistency across all targets.
add_library(friendlist_build_options INTERFACE)

if (MSVC)
    # CMake injects /EHsc by default for MSVC toolchains.
    # Ashita SDK uses _set_se_translator (SEH -> C++ exception translation) which requires /EHa,
    # and /EHa + /EHsc triggers noisy command-line warning D9025.
    #
    # Fix: Strip /EHsc from the global MSVC flags and then opt into /EHa explicitly.
    foreach(flag_var
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
    )
        if (DEFINED ${flag_var})
            string(REPLACE "/EHsc" "" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    # /MP: Enable multi-processor compilation (parallel compilation of multiple .cpp files)
    # /FS: Allow multiple CL.EXE processes to write to the same PDB file simultaneously
    # /EHa: Enable C++ exception handling with SEH (Required by Ashita SDK ErrorHandling.h)
    # /W4:  Warning level 4
    # /permissive-: Standards compliance
    target_compile_options(friendlist_build_options INTERFACE
        /MP
        /FS
        /EHa
        /W4
        /permissive-
        /wd4100
    )

    # ==================================================
    # Debug build flags (use test server)
    # ==================================================
    # Define USE_TEST_SERVER for Debug builds to automatically route to test API
    target_compile_definitions(friendlist_build_options INTERFACE
        $<$<CONFIG:Debug>:USE_TEST_SERVER>
    )
    
    # ==================================================
    # Optimal Release flags (speed + smaller DLL)
    # ==================================================
    # These are applied only for Release builds.
    target_compile_options(friendlist_build_options INTERFACE
        $<$<CONFIG:Release>:/O2>     # Maximize speed
        $<$<CONFIG:Release>:/Ob2>    # Aggressive inlining
        $<$<CONFIG:Release>:/Oi>     # Intrinsics
        $<$<CONFIG:Release>:/Ot>     # Favor speed
        $<$<CONFIG:Release>:/Gy>     # Function-level linking
        $<$<CONFIG:Release>:/GF>     # String pooling
        $<$<CONFIG:Release>:/DNDEBUG>
    )

    target_link_options(friendlist_build_options INTERFACE
        $<$<CONFIG:Release>:/OPT:REF>  # Remove unused code/data
        $<$<CONFIG:Release>:/OPT:ICF>  # Fold identical functions
    )

    # Optional: keep symbols in Release for crash analysis (perf impact usually negligible)
    # target_link_options(friendlist_build_options INTERFACE $<$<CONFIG:Release>:/DEBUG>)

    # NOTE: LTCG (/GL + /LTCG) intentionally NOT enabled by default here.
    # If you want maximum optimization and accept slower links, add:
    # target_compile_options(friendlist_build_options INTERFACE $<$<CONFIG:Release>:/GL>)
    # target_link_options(friendlist_build_options INTERFACE $<$<CONFIG:Release>:/LTCG>)
else()
    # GCC/Clang warning flags
    target_compile_options(friendlist_build_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ==================================================
# Generate version header from CMake project version
# ==================================================
# Calculate double version for Ashita API (major.minor.patch format as decimal)
# e.g., 0.6.1 -> 0.61, 1.2.3 -> 1.23
# Formula: major.minorpatch as decimal (e.g., 0.61 for 0.6.1, 1.23 for 1.2.3)
# Since CMake math() only works with integers, we format as string
# Format: "major.minorpatch" where minorpatch is minor*10 + patch (e.g., 6*10+1=61)
math(EXPR MINORPATCH "${PROJECT_VERSION_MINOR} * 10 + ${PROJECT_VERSION_PATCH}")
if(PROJECT_VERSION_MAJOR EQUAL 0)
    # For versions < 1.0, format as "0.minorpatch" (e.g., 0.6.1 -> 0.61)
    set(PLUGIN_VERSION_DOUBLE "0.${MINORPATCH}")
else()
    # For versions >= 1.0, format as "major.minorpatch" (e.g., 1.2.3 -> 1.23)
    set(PLUGIN_VERSION_DOUBLE "${PROJECT_VERSION_MAJOR}.${MINORPATCH}")
endif()

# Generate PluginVersion.h from template
configure_file(
    ${CMAKE_SOURCE_DIR}/include/PluginVersion.h.in
    ${CMAKE_BINARY_DIR}/include/PluginVersion.h
    @ONLY
)

# Add generated include directory
set(GENERATED_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)

# ==================================================
# Common include directories
# ==================================================
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GENERATED_INCLUDE_DIR}
)

# ==================================================
# Layer 1: Core Library
# ==================================================
set(CORE_SOURCES
    src/Core/FriendsCore.cpp
    src/Core/ModelsCore.cpp
    src/Core/NotesCore.cpp
    src/Core/UtilitiesCore.cpp
    src/Core/VersionCore.cpp
)

add_library(friendlist_core STATIC ${CORE_SOURCES})

target_include_directories(friendlist_core
    PUBLIC
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(friendlist_core
    PUBLIC
        friendlist_build_options
)

# ==================================================
# Layer 2: Protocol Library
# ==================================================
set(PROTOCOL_SOURCES
    src/Protocol/ProtocolVersion.cpp
    src/Protocol/MessageTypes.cpp
    src/Protocol/JsonUtils.cpp
    src/Protocol/RequestEncoder.cpp
    src/Protocol/ResponseDecoder.cpp
    src/Protocol/MessageValidator.cpp
    src/Protocol/HttpHeaders.cpp
)

add_library(friendlist_protocol STATIC ${PROTOCOL_SOURCES})

target_include_directories(friendlist_protocol
    PUBLIC
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(friendlist_protocol
    PUBLIC
        friendlist_core
        friendlist_build_options
)

# ==================================================
# Layer 3: App Library
# ==================================================
set(APP_SOURCES
    src/App/StateMachines/ConnectionState.cpp
    src/App/State/NotesState.cpp
    src/App/State/ThemeState.cpp
    src/App/UseCases/ConnectionUseCases.cpp
    src/App/UseCases/FriendsUseCases.cpp
    src/App/UseCases/ServerListUseCases.cpp
    src/App/UseCases/ThemingUseCases.cpp
    src/App/UseCases/NotificationUseCases.cpp
    src/App/UseCases/PreferencesUseCases.cpp
    src/App/UseCases/NotesUseCases.cpp
    src/App/UseCases/TestRunnerUseCase.cpp
    src/App/SoundResolver.cpp
    src/App/NotificationSoundService.cpp
)

add_library(friendlist_app STATIC ${APP_SOURCES})

# Make the app library depend on the generated embedded resources (for SoundResolver)
add_dependencies(friendlist_app embedded_resources_target)

target_include_directories(friendlist_app
    PUBLIC
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(friendlist_app
    PUBLIC
        friendlist_core
        friendlist_protocol
        friendlist_build_options
)

# ==================================================
# Layer 4: UI Library
# ==================================================
set(UI_SOURCES
    src/UI/Interfaces/UiRenderer.cpp
    src/UI/Helpers/TooltipHelper.cpp
    src/UI/Helpers/WindowHelper.cpp
    src/UI/Widgets/Controls.cpp
    src/UI/Widgets/Inputs.cpp
    src/UI/Widgets/Indicators.cpp
    src/UI/Widgets/Layout.cpp
    src/UI/Widgets/Tables.cpp
    src/UI/Notifications/ToastManager.cpp
    src/UI/ViewModels/FriendListViewModel.cpp
    src/UI/ViewModels/ThemesViewModel.cpp
    src/UI/ViewModels/OptionsViewModel.cpp
    src/UI/ViewModels/AltVisibilityViewModel.cpp
    src/Debug/DebugLog.cpp
    src/Debug/Perf.cpp
    src/UI/Windows/QuickOnlineWindow.cpp
    src/UI/Windows/WindowManager.cpp
    src/UI/Windows/MainWindow.cpp
    src/UI/Windows/ServerSelectionWindow.cpp
    src/UI/Windows/ThemesWindow.cpp
    src/UI/Windows/DebugLogWindow.cpp
    src/UI/Windows/WindowClosePolicy.cpp
    src/UI/Windows/UiCloseCoordinator.cpp
    src/UI/ViewModels/NotesViewModel.cpp
    src/UI/Windows/NoteEditorWindow.cpp
    src/UI/ViewModels/MailViewModel.cpp
)

add_library(friendlist_ui STATIC ${UI_SOURCES})

target_include_directories(friendlist_ui
    PUBLIC
        ${ASHITA_SDK_DIR}  # Must come BEFORE COMMON_INCLUDE_DIRS to ensure SDK imgui.h is found first
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(friendlist_ui
    PUBLIC
        friendlist_core
        friendlist_protocol
        friendlist_app
        friendlist_build_options
)

# ==================================================
# Generate Embedded Resources (Images + Sounds)
# ==================================================
# This runs before building to ensure EmbeddedResources.h is up to date
set(EMBEDDED_RESOURCES_OUTPUT ${CMAKE_SOURCE_DIR}/src/Platform/Ashita/EmbeddedResources.h)
set(CONVERT_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/convert_sounds_to_wav.py)
set(GENERATE_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/generate_embedded.py)

# Find Python interpreter
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Custom command to generate embedded resources
# First converts MP3 to WAV (if needed and ffmpeg is available), then generates the header
# Note: If WAV files already exist, the convert script will skip conversion gracefully
add_custom_command(
    OUTPUT ${EMBEDDED_RESOURCES_OUTPUT}
    COMMAND ${Python3_EXECUTABLE} "${CONVERT_SCRIPT}"
    COMMAND ${Python3_EXECUTABLE} "${GENERATE_SCRIPT}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating embedded resources (converting sounds and embedding assets)..."
    DEPENDS
        ${CMAKE_SOURCE_DIR}/assets/icons/online.png
        ${CMAKE_SOURCE_DIR}/assets/icons/friend_request.png
        ${CMAKE_SOURCE_DIR}/assets/sounds/online.mp3
        ${CMAKE_SOURCE_DIR}/assets/sounds/friend-request.mp3
        ${CONVERT_SCRIPT}
        ${GENERATE_SCRIPT}
    VERBATIM
)

# Create a custom target for embedded resources so we can depend on it
add_custom_target(embedded_resources_target
    DEPENDS ${EMBEDDED_RESOURCES_OUTPUT}
    COMMENT "Target for embedded resources generation"
)

# ==================================================
# Layer 5: Platform/Ashita Library (Plugin Only)
# ==================================================
set(PLATFORM_ASHITA_SOURCES
    src/Platform/Ashita/AshitaNetClient.cpp
    src/Platform/Ashita/AshitaClock.cpp
    src/Platform/Ashita/AshitaLogger.cpp
    src/Platform/Ashita/ThemePersistence.cpp
    src/Platform/Ashita/ServerSelectionPersistence.cpp
    src/Platform/Ashita/CacheMigration.cpp
    src/Platform/Ashita/PathUtils.cpp
    src/Platform/Ashita/AshitaPreferencesStore.cpp
    src/Platform/Ashita/NotesPersistence.cpp
    src/Platform/Ashita/ApiKeyPersistence.cpp
    src/Platform/Ashita/AshitaThemeHelper.cpp
    src/Platform/Ashita/ImGuiBridge.cpp
    src/Platform/Ashita/AshitaNotifiedMailStore.cpp
    src/Platform/Ashita/AshitaMailStore.cpp
    src/Platform/Ashita/AshitaSoundPlayer.cpp
)

list(APPEND PLATFORM_ASHITA_SOURCES
    src/Platform/Ashita/AshitaUiRenderer.cpp
    src/Platform/Ashita/IconManager.cpp
    src/Platform/Ashita/PatternScanFingerprint.cpp
    src/Platform/Ashita/AshitaAdapter.cpp
    src/Platform/Ashita/AshitaEventQueue.cpp
    src/Platform/Ashita/CommandHandlerHook.cpp
    src/Platform/Ashita/FriendListMenuDetector.cpp
    src/Platform/Ashita/MenuHook.cpp
    src/Platform/Ashita/KeyEdgeDetector.cpp
    src/Platform/Ashita/AshitaRealmDetector.cpp
    src/Platform/Ashita/XIFriendList.cpp
)

add_library(friendlist_platform_ashita STATIC ${PLATFORM_ASHITA_SOURCES})

# Make the platform library depend on the generated embedded resources
add_dependencies(friendlist_platform_ashita embedded_resources_target)

target_include_directories(friendlist_platform_ashita
    PUBLIC
        ${ASHITA_SDK_DIR}  # Must come BEFORE COMMON_INCLUDE_DIRS to ensure SDK imgui.h is found first
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(friendlist_platform_ashita
    PUBLIC
        friendlist_core
        friendlist_protocol
        friendlist_app
        friendlist_ui
        friendlist_build_options
)

# Suppress stb_image.h warnings (C4505)
if (MSVC)
    set_source_files_properties(
        src/Platform/Ashita/IconManager.cpp
        PROPERTIES
        COMPILE_FLAGS "/wd4505"
    )
endif()

# ==================================================
# Plugin Target (links all libraries)
# ==================================================
add_library(FFXIFriendList SHARED
    src/Platform/Ashita/PluginEntryPoint.cpp
)

target_include_directories(FFXIFriendList
    PRIVATE
        ${ASHITA_SDK_DIR}  # Must come BEFORE COMMON_INCLUDE_DIRS to ensure SDK imgui.h is found first
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(FFXIFriendList
    PRIVATE
        friendlist_core
        friendlist_protocol
        friendlist_app
        friendlist_ui
        friendlist_platform_ashita
        friendlist_build_options
)

# ==================================================
# Windows DLL naming (no lib prefix) and exports
# ==================================================
if (WIN32)
    set_target_properties(FFXIFriendList PROPERTIES
        PREFIX ""
        OUTPUT_NAME "FFXIFriendList"
    )
    set_target_properties(FFXIFriendList PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/Platform/Ashita/FFXIFriendList.def"
    )
endif()

# ==================================================
# Tests
# ==================================================
if (BUILD_TESTS)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)

    # Catch2 builds as its own targets and does not consume our build-options interface library.
    # Because we strip /EHsc globally (to avoid D9025 noise when using /EHa for Ashita),
    # we explicitly enable standard C++ exception unwind semantics for Catch2 to prevent C4530 spam.
    if (MSVC)
        if (TARGET Catch2)
            target_compile_options(Catch2 PRIVATE /EHsc)
        endif()
        if (TARGET Catch2WithMain)
            target_compile_options(Catch2WithMain PRIVATE /EHsc)
        endif()
    endif()

    add_executable(FFXIFriendList_tests
        tests/Core/FriendTest.cpp
        tests/Core/FriendStatusTest.cpp
        tests/Core/PresenceTest.cpp
        tests/Core/FriendListTest.cpp
        tests/Core/FriendListSorterTest.cpp
        tests/Core/FriendListFilterTest.cpp
        tests/Core/FriendListDifferTest.cpp
        tests/Core/NoteMergerTest.cpp
        tests/Core/SanitizeTest.cpp
        tests/Core/MailTest.cpp
        tests/Core/NotificationSoundPolicyTest.cpp
        tests/Core/VersionTest.cpp
        tests/Core/ThemeTest.cpp
        tests/Core/PreferencesTest.cpp
        tests/Core/MemoryStatsTest.cpp
        tests/Protocol/ProtocolVersionTest.cpp
        tests/Protocol/JsonUtilsTest.cpp
        tests/Protocol/MessageTypesTest.cpp
        tests/Protocol/RequestEncoderTest.cpp
        tests/Protocol/ResponseDecoderTest.cpp
        tests/Protocol/MessageValidatorTest.cpp
        tests/Protocol/MalformedInputTest.cpp
        tests/Protocol/HttpHeadersTest.cpp
        tests/App/ConnectionStateTest.cpp
        tests/App/ConnectUseCaseTest.cpp
        tests/App/SyncFriendListUseCaseTest.cpp
        tests/App/AcceptFriendRequestUseCaseTest.cpp
        tests/App/RejectFriendRequestUseCaseTest.cpp
        tests/App/CancelFriendRequestUseCaseTest.cpp
        tests/App/UpdatePresenceUseCaseTest.cpp
        tests/App/UpdateMyStatusUseCaseTest.cpp
        tests/App/GetAltVisibilityUseCaseTest.cpp
        tests/App/PreferencesUseCaseTest.cpp
        tests/App/NotificationUseCaseTest.cpp
        tests/App/NotificationSoundServiceTest.cpp
        tests/App/SoundResolverTest.cpp
        tests/App/RealmDetectorTest.cpp
        tests/App/RemoveFriendUseCaseTest.cpp
        tests/App/RemoveFriendVisibilityUseCaseTest.cpp
        tests/App/SendFriendRequestUseCaseTest.cpp
        tests/App/GetNotesUseCaseTest.cpp
        tests/App/SaveNoteUseCaseTest.cpp
        tests/App/DeleteNoteUseCaseTest.cpp
        tests/App/UrlConstructionTest.cpp
        tests/App/GetFriendRequestsUseCaseTest.cpp
        tests/App/RefreshBehaviorTest.cpp
        tests/App/ThemeUseCaseTest.cpp
        tests/UI/FriendListViewModelTest.cpp
        tests/UI/FriendListWindowTest.cpp
        tests/UI/ThemesViewModelTest.cpp
        tests/UI/OptionsViewModelTest.cpp
        tests/UI/NotesViewModelTest.cpp
        tests/UI/AltVisibilityViewModelTest.cpp
        tests/UI/WindowClosePolicyTest.cpp
        tests/Platform/EchoMessagesTest.cpp
        tests/Platform/CommandParsingTest.cpp
        tests/Platform/CommandHandlerHookTest.cpp
        tests/Platform/KeyEdgeDetectorTest.cpp
        tests/Platform/PollingCadenceTest.cpp
        tests/Debug/DebugLogTest.cpp
    )

    target_include_directories(FFXIFriendList_tests
        PRIVATE
            ${COMMON_INCLUDE_DIRS}
    )

    target_compile_definitions(FFXIFriendList_tests PRIVATE BUILDING_TESTS)

    target_link_libraries(FFXIFriendList_tests
        PRIVATE
            Catch2::Catch2WithMain
            friendlist_core
            friendlist_protocol
            friendlist_app
            friendlist_ui
            friendlist_platform_ashita
            friendlist_build_options
    )

    include(Catch)
    catch_discover_tests(FFXIFriendList_tests)
endif()
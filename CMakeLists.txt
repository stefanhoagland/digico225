cmake_minimum_required(VERSION 3.22)

project(OpenDesk VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build a universal Mac app (Apple Silicon + Intel) unless told otherwise.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

set(OPENDESK_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    OPENDESK_VERSION="${PROJECT_VERSION}")

# The audio engine is compiled into both the app and the tests. (JUCE modules
# are source-level libraries, so we don't wrap the engine in a static lib.)
set(OPENDESK_ENGINE_SOURCES
    Source/Engine/MixerEngine.cpp)

# ---------------------------------------------------------------------------
# The app
# ---------------------------------------------------------------------------
juce_add_gui_app(OpenDesk
    PRODUCT_NAME                 "OpenDesk"
    COMPANY_NAME                 "OpenDesk"
    BUNDLE_ID                    "com.opendesk.app"
    VERSION                      ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT   "OpenDesk needs audio input access to pass audio from your interface through the mixer.")

target_sources(OpenDesk PRIVATE
    ${OPENDESK_ENGINE_SOURCES}
    Source/Main.cpp
    Source/UI/MainComponent.cpp
    Source/UI/ChannelStripComponent.cpp
    Source/UI/MasterStripComponent.cpp
    Source/UI/DeskLookAndFeel.cpp)

target_include_directories(OpenDesk PRIVATE Source)
target_compile_definitions(OpenDesk PRIVATE ${OPENDESK_DEFINES})
target_link_libraries(OpenDesk
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Tests: run the engine with fake audio buffers, no hardware needed
# ---------------------------------------------------------------------------
enable_testing()
juce_add_console_app(EngineTests PRODUCT_NAME "EngineTests")
target_sources(EngineTests PRIVATE ${OPENDESK_ENGINE_SOURCES} Tests/EngineTests.cpp)
target_include_directories(EngineTests PRIVATE Source)
target_compile_definitions(EngineTests PRIVATE ${OPENDESK_DEFINES})
target_link_libraries(EngineTests PRIVATE
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
add_test(NAME EngineTests COMMAND EngineTests)

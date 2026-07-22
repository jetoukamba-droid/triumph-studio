cmake_minimum_required(VERSION 3.22)

project(TriumphStudio VERSION 0.22.0 LANGUAGES CXX)

set(TRIUMPH_RELEASE_LABEL "0.22.0-beta.1")
set(TRIUMPH_RELEASE_CHANNEL "Closed Beta 1")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(TRIUMPH_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout")

if (TRIUMPH_JUCE_PATH)
    add_subdirectory("${TRIUMPH_JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.13
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_gui_app(TriumphStudio
    PRODUCT_NAME "Triumph Studio"
    COMPANY_NAME "Triumph Platform LLC"
    BUNDLE_ID "com.triumphplatform.studio"
    VERSION "${PROJECT_VERSION}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/TriumphStudioIcon-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/TriumphStudioIcon-256.png"
)

juce_generate_juce_header(TriumphStudio)

target_sources(TriumphStudio PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/MainComponent.h
    src/audio/AudioEngine.cpp
    src/audio/AudioEngine.h
    src/audio/AdvancedAudioRenderer.cpp
    src/audio/AdvancedAudioRenderer.h
    src/audio/AudioRecorder.cpp
    src/audio/AudioRecorder.h
    src/audio/MultiTrackRecorder.cpp
    src/audio/MultiTrackRecorder.h
    src/audio/RecordingJournal.cpp
    src/audio/RecordingJournal.h
    src/audio/OfflineRenderer.cpp
    src/audio/OfflineRenderer.h
    src/audio/PluginFreezeRenderer.cpp
    src/audio/PluginFreezeRenderer.h
    src/audio/StemExporter.cpp
    src/audio/StemExporter.h
    src/audio/AudioTrack.cpp
    src/audio/AudioTrack.h
    src/core/TimelineMath.h
    src/core/TimeStretch.h
    src/core/WarpMap.h
    src/core/TempoMap.h
    src/core/TempoAutomation.h
    src/core/AdvancedAudioEdit.h
    src/core/ProducerAssistant.h
    src/core/HelpAssistant.h
    src/core/ResponsiveLayout.h
    src/core/BuiltInInstrument.h
    src/core/ParameterSmoother.h
    src/core/MidiCapture.h
    src/core/OfflineRenderMath.h
    src/core/PluginGraph.h
    src/core/SampleDelayLine.h
    src/core/MixerGraph.h
    src/core/MixerMath.h
    src/core/RealtimeEngineState.h
    src/core/RealtimeDiagnosticsReport.h
    src/core/FixedSampleClock.h
    src/core/RecordingStateMachine.h
    src/core/RecordingPlan.h
    src/core/RecordingTypes.h
    src/core/MonitoringCapabilities.h
    src/core/AudioOutputHealth.h
    src/core/AudioDeviceRecovery.h
    src/core/KeyboardShortcuts.h
    src/core/ReleaseIdentity.h
    src/core/PluginHostPolicy.h
    src/core/PluginRack.h
    src/core/PluginRuntimeIsolation.h
    src/core/MidiPerformance.h
    src/core/AutomationWrite.h
    src/model/ProjectModel.cpp
    src/model/ProjectModel.h
    src/project/ProjectDocument.cpp
    src/project/ProjectDocument.h
    src/project/ProjectWorkspace.cpp
    src/project/ProjectWorkspace.h
    src/plugin/PluginScanService.cpp
    src/plugin/PluginScanService.h
    src/plugin/PluginPresetStore.cpp
    src/plugin/PluginPresetStore.h
    src/ui/ArrangementComponent.cpp
    src/ui/ArrangementComponent.h
    src/ui/AdvancedEditComponent.cpp
    src/ui/AdvancedEditComponent.h
    src/ui/ProducerAssistantComponent.cpp
    src/ui/ProducerAssistantComponent.h
    src/ui/HelpAssistantComponent.cpp
    src/ui/HelpAssistantComponent.h
    src/ui/PianoRollComponent.cpp
    src/ui/PianoRollComponent.h
    src/ui/StudioLookAndFeel.cpp
    src/ui/StudioLookAndFeel.h
    src/ui/StudioIconButton.h
    src/ui/TrackRowComponent.cpp
    src/ui/TrackRowComponent.h
    src/ui/MixerComponent.cpp
    src/ui/MixerComponent.h
    src/ui/TempoAutomationComponent.cpp
    src/ui/TempoAutomationComponent.h
)

target_include_directories(TriumphStudio PRIVATE src)

target_compile_definitions(TriumphStudio PRIVATE
    JUCE_ASIO=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(TriumphStudio PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_add_console_app(TriumphPluginScanner
    PRODUCT_NAME "Triumph Plugin Scanner"
    COMPANY_NAME "Triumph Platform LLC"
    BUNDLE_ID "com.triumphplatform.studio.pluginscanner"
    VERSION "${PROJECT_VERSION}"
)

juce_generate_juce_header(TriumphPluginScanner)

target_sources(TriumphPluginScanner PRIVATE
    src/plugin/PluginScannerMain.cpp
)

target_compile_definitions(TriumphPluginScanner PRIVATE
    JUCE_ASIO=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(TriumphPluginScanner PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors_headless
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

add_dependencies(TriumphStudio TriumphPluginScanner)
add_custom_command(TARGET TriumphStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:TriumphPluginScanner>"
        "$<TARGET_FILE_DIR:TriumphStudio>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:TriumphPluginScanner>"
        "$<TARGET_FILE_DIR:TriumphStudio>/Triumph Plugin Scanner.exe"
    VERBATIM
)

install(TARGETS TriumphStudio TriumphPluginScanner
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
    RESOURCE DESTINATION resources
)

install(FILES
    BETA_README.txt
    RELEASE_NOTES.md
    KNOWN_ISSUES.md
    docs/Beta1Acceptance.md
    tools/LaunchTriumphStudio.ps1
    DESTINATION .
)

include(CTest)

if (BUILD_TESTING)
    add_executable(ReleaseIdentityTests tests/ReleaseIdentityTests.cpp)
    target_include_directories(ReleaseIdentityTests PRIVATE src tests)
    add_test(NAME ReleaseIdentityTests COMMAND ReleaseIdentityTests)

    add_executable(KeyboardShortcutsTests tests/KeyboardShortcutsTests.cpp)
    target_include_directories(KeyboardShortcutsTests PRIVATE src tests)
    add_test(NAME KeyboardShortcutsTests COMMAND KeyboardShortcutsTests)

    add_executable(AudioDeviceRecoveryTests tests/AudioDeviceRecoveryTests.cpp)
    target_include_directories(AudioDeviceRecoveryTests PRIVATE src tests)
    add_test(NAME AudioDeviceRecoveryTests COMMAND AudioDeviceRecoveryTests)

    add_executable(AudioOutputHealthTests tests/AudioOutputHealthTests.cpp)
    target_include_directories(AudioOutputHealthTests PRIVATE src tests)
    add_test(NAME AudioOutputHealthTests COMMAND AudioOutputHealthTests)

    add_executable(TimelineMathTests tests/TimelineMathTests.cpp)
    target_include_directories(TimelineMathTests PRIVATE src)
    add_test(NAME TimelineMathTests COMMAND TimelineMathTests)

    add_executable(TempoMapTests tests/TempoMapTests.cpp)
    target_include_directories(TempoMapTests PRIVATE src)
    add_test(NAME TempoMapTests COMMAND TempoMapTests)

    add_executable(ParameterSmootherTests tests/ParameterSmootherTests.cpp)
    target_include_directories(ParameterSmootherTests PRIVATE src)
    add_test(NAME ParameterSmootherTests COMMAND ParameterSmootherTests)

    add_executable(MidiCaptureTests tests/MidiCaptureTests.cpp)
    target_include_directories(MidiCaptureTests PRIVATE src)
    add_test(NAME MidiCaptureTests COMMAND MidiCaptureTests)

    add_executable(PluginGraphTests tests/PluginGraphTests.cpp)
    target_include_directories(PluginGraphTests PRIVATE src)
    add_test(NAME PluginGraphTests COMMAND PluginGraphTests)

    add_executable(PluginHostPolicyTests tests/PluginHostPolicyTests.cpp)
    target_include_directories(PluginHostPolicyTests PRIVATE src tests)
    add_test(NAME PluginHostPolicyTests COMMAND PluginHostPolicyTests)

    add_executable(PluginRuntimeIsolationTests
        tests/PluginRuntimeIsolationTests.cpp)
    target_include_directories(PluginRuntimeIsolationTests PRIVATE src tests)
    add_test(NAME PluginRuntimeIsolationTests
        COMMAND PluginRuntimeIsolationTests)

    add_executable(ProfessionalMixerGraphTests
        tests/ProfessionalMixerGraphTests.cpp)
    target_include_directories(ProfessionalMixerGraphTests PRIVATE src tests)
    add_test(NAME ProfessionalMixerGraphTests
        COMMAND ProfessionalMixerGraphTests)

    add_executable(MidiPerformanceTests tests/MidiPerformanceTests.cpp)
    target_include_directories(MidiPerformanceTests PRIVATE src tests)
    add_test(NAME MidiPerformanceTests COMMAND MidiPerformanceTests)

    add_executable(AutomationWriteTests tests/AutomationWriteTests.cpp)
    target_include_directories(AutomationWriteTests PRIVATE src tests)
    add_test(NAME AutomationWriteTests COMMAND AutomationWriteTests)

    add_executable(PluginPresetStoreTests
        tests/PluginPresetStoreTests.cpp
        src/plugin/PluginPresetStore.cpp
        src/plugin/PluginPresetStore.h
    )
    target_include_directories(PluginPresetStoreTests PRIVATE src tests)
    target_link_libraries(PluginPresetStoreTests PRIVATE juce::juce_core)
    add_test(NAME PluginPresetStoreTests COMMAND PluginPresetStoreTests)

    add_executable(SampleDelayLineTests tests/SampleDelayLineTests.cpp)
    target_include_directories(SampleDelayLineTests PRIVATE src)
    add_test(NAME SampleDelayLineTests COMMAND SampleDelayLineTests)

    add_executable(MixerGraphTests tests/MixerGraphTests.cpp)
    target_include_directories(MixerGraphTests PRIVATE src)
    add_test(NAME MixerGraphTests COMMAND MixerGraphTests)

    add_executable(TempoAutomationTests tests/TempoAutomationTests.cpp)
    target_include_directories(TempoAutomationTests PRIVATE src)
    add_test(NAME TempoAutomationTests COMMAND TempoAutomationTests)

    add_executable(AdvancedAudioEditTests tests/AdvancedAudioEditTests.cpp)
    target_include_directories(AdvancedAudioEditTests PRIVATE src)
    add_test(NAME AdvancedAudioEditTests COMMAND AdvancedAudioEditTests)

    add_executable(ProducerAssistantTests tests/ProducerAssistantTests.cpp)
    target_include_directories(ProducerAssistantTests PRIVATE src)
    add_test(NAME ProducerAssistantTests COMMAND ProducerAssistantTests)

    add_executable(HelpAssistantTests tests/HelpAssistantTests.cpp)
    target_include_directories(HelpAssistantTests PRIVATE src)
    add_test(NAME HelpAssistantTests COMMAND HelpAssistantTests)

    add_executable(ResponsiveLayoutTests tests/ResponsiveLayoutTests.cpp)
    target_include_directories(ResponsiveLayoutTests PRIVATE src)
    add_test(NAME ResponsiveLayoutTests COMMAND ResponsiveLayoutTests)

    add_executable(RealtimeEngineStateTests tests/RealtimeEngineStateTests.cpp)
    target_include_directories(RealtimeEngineStateTests PRIVATE src)
    add_test(NAME RealtimeEngineStateTests COMMAND RealtimeEngineStateTests)

    add_executable(FixedSampleClockTests tests/FixedSampleClockTests.cpp)
    target_include_directories(FixedSampleClockTests PRIVATE src)
    add_test(NAME FixedSampleClockTests COMMAND FixedSampleClockTests)

    add_executable(RecordingStateMachineTests tests/RecordingStateMachineTests.cpp)
    target_include_directories(RecordingStateMachineTests PRIVATE src)
    add_test(NAME RecordingStateMachineTests COMMAND RecordingStateMachineTests)

    add_executable(RecordingPlanTests tests/RecordingPlanTests.cpp)
    target_include_directories(RecordingPlanTests PRIVATE src tests)
    add_test(NAME RecordingPlanTests COMMAND RecordingPlanTests)

    add_executable(MonitoringCapabilitiesTests
        tests/MonitoringCapabilitiesTests.cpp)
    target_include_directories(MonitoringCapabilitiesTests PRIVATE src tests)
    add_test(NAME MonitoringCapabilitiesTests COMMAND MonitoringCapabilitiesTests)

    add_executable(RecordingJournalTests
        tests/RecordingJournalTests.cpp
        src/audio/RecordingJournal.cpp
        src/audio/RecordingJournal.h
    )
    target_include_directories(RecordingJournalTests PRIVATE src)
    target_link_libraries(RecordingJournalTests PRIVATE juce::juce_core)
    add_test(NAME RecordingJournalTests COMMAND RecordingJournalTests)

    add_executable(MultiTrackRecorderTests
        tests/MultiTrackRecorderTests.cpp
        src/audio/MultiTrackRecorder.cpp
        src/audio/MultiTrackRecorder.h
        src/audio/AudioRecorder.cpp
        src/audio/AudioRecorder.h
        src/audio/RecordingJournal.cpp
        src/audio/RecordingJournal.h
    )
    target_include_directories(MultiTrackRecorderTests PRIVATE src tests)
    target_link_libraries(MultiTrackRecorderTests PRIVATE
        juce::juce_audio_formats
        juce::juce_core
    )
    add_test(NAME MultiTrackRecorderTests COMMAND MultiTrackRecorderTests)

    add_executable(PluginScanServiceTests
        tests/PluginScanServiceTests.cpp
        src/plugin/PluginScanService.cpp
        src/plugin/PluginScanService.h
    )
    add_executable(PluginScannerFailureStub
        tests/PluginScannerFailureStub.cpp
    )
    target_include_directories(PluginScanServiceTests PRIVATE src)
    target_link_libraries(PluginScanServiceTests PRIVATE
        juce::juce_audio_processors_headless
        juce::juce_core
    )
    add_dependencies(PluginScanServiceTests
        PluginScannerFailureStub
        TriumphPluginScanner
    )
    add_test(NAME PluginScanServiceTests
        COMMAND $<TARGET_FILE:PluginScanServiceTests>
            "$<TARGET_FILE:PluginScannerFailureStub>"
            "$<TARGET_FILE:TriumphPluginScanner>"
    )

    add_test(
        NAME RealtimeCallbackAudit
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/audio/AudioEngine.cpp
            -DRECORDER_SOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/audio/AudioRecorder.cpp
            -DMULTITRACK_RECORDER_SOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/audio/MultiTrackRecorder.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/RealtimeCallbackAudit.cmake
    )

    juce_add_console_app(AudioEngineRealtimeTests
        PRODUCT_NAME "AudioEngineRealtimeTests"
    )
    target_sources(AudioEngineRealtimeTests PRIVATE
        tests/AudioEngineRealtimeTests.cpp
        src/audio/AudioEngine.cpp
        src/audio/AudioEngine.h
        src/audio/AudioTrack.cpp
        src/audio/AudioTrack.h
    )
    juce_generate_juce_header(AudioEngineRealtimeTests)
    target_include_directories(AudioEngineRealtimeTests PRIVATE src)
    target_compile_definitions(AudioEngineRealtimeTests PRIVATE
        JUCE_ASIO=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
    )
    target_link_libraries(AudioEngineRealtimeTests PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
    )
    add_test(NAME AudioEngineRealtimeTests COMMAND AudioEngineRealtimeTests)

    add_executable(ProjectModelTests
        tests/ProjectModelTests.cpp
        src/model/ProjectModel.cpp
        src/model/ProjectModel.h
        src/project/ProjectDocument.cpp
        src/project/ProjectDocument.h
        src/audio/OfflineRenderer.cpp
        src/audio/OfflineRenderer.h
        src/audio/StemExporter.cpp
        src/audio/StemExporter.h
    )
    target_include_directories(ProjectModelTests PRIVATE src)
    target_link_libraries(ProjectModelTests PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_graphics
        juce::juce_audio_formats
        juce::juce_audio_processors
    )
    add_test(NAME ProjectModelTests COMMAND ProjectModelTests)
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "Triumph Studio")
set(CPACK_PACKAGE_VENDOR "Triumph Platform LLC")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "Triumph-Studio-${TRIUMPH_RELEASE_LABEL}-Windows-x64")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_MONOLITHIC_INSTALL ON)
set(CPACK_PACKAGE_CHECKSUM "SHA256")
include(CPack)

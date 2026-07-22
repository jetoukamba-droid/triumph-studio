if (NOT DEFINED SOURCE_FILE)
    message(FATAL_ERROR "SOURCE_FILE is required")
endif()
if (NOT DEFINED RECORDER_SOURCE_FILE)
    message(FATAL_ERROR "RECORDER_SOURCE_FILE is required")
endif()
if (NOT DEFINED MULTITRACK_RECORDER_SOURCE_FILE)
    message(FATAL_ERROR "MULTITRACK_RECORDER_SOURCE_FILE is required")
endif()

file(READ "${SOURCE_FILE}" source_text)
set(start_marker "void AudioEngine::getNextAudioBlock")
set(end_marker "AudioEngine::TrackDelayState* AudioEngine::findTrackDelayState")
string(FIND "${source_text}" "${start_marker}" start_position)
if (start_position EQUAL -1)
    message(FATAL_ERROR "Audio callback start marker was not found")
endif()

string(SUBSTRING "${source_text}" ${start_position} -1 callback_tail)
string(FIND "${callback_tail}" "${end_marker}" end_position)
if (end_position EQUAL -1)
    message(FATAL_ERROR "Audio callback end marker was not found")
endif()
string(SUBSTRING "${callback_tail}" 0 ${end_position} callback_text)

set(forbidden_tokens
    "ScopedLock"
    "ScopedTryLock"
    "CriticalSection"
    "std::mutex"
    "setSize ("
    "createReaderFor"
    "make_unique"
    "push_back"
    "File::"
    "Logger::"
    "std::cout"
)

foreach (token IN LISTS forbidden_tokens)
    string(FIND "${callback_text}" "${token}" found_position)
    if (NOT found_position EQUAL -1)
        message(FATAL_ERROR
            "Real-time callback contains forbidden operation token: ${token}")
    endif()
endforeach()

file(READ "${RECORDER_SOURCE_FILE}" recorder_source_text)
set(recorder_start_marker "void AudioRecorder::processInput")
set(recorder_end_marker "AudioRecorder::Result AudioRecorder::stop")
string(FIND "${recorder_source_text}" "${recorder_start_marker}"
       recorder_start_position)
if (recorder_start_position EQUAL -1)
    message(FATAL_ERROR "Recording callback start marker was not found")
endif()
string(SUBSTRING "${recorder_source_text}" ${recorder_start_position} -1
       recorder_callback_tail)
string(FIND "${recorder_callback_tail}" "${recorder_end_marker}"
       recorder_end_position)
if (recorder_end_position EQUAL -1)
    message(FATAL_ERROR "Recording callback end marker was not found")
endif()
string(SUBSTRING "${recorder_callback_tail}" 0 ${recorder_end_position}
       recorder_callback_text)
foreach (token IN LISTS forbidden_tokens)
    string(FIND "${recorder_callback_text}" "${token}" found_position)
    if (NOT found_position EQUAL -1)
        message(FATAL_ERROR
            "Recording callback contains forbidden operation token: ${token}")
    endif()
endforeach()

file(READ "${MULTITRACK_RECORDER_SOURCE_FILE}" multitrack_source_text)
set(multitrack_start_marker "void MultiTrackRecorder::processSpan")
set(multitrack_end_marker "void MultiTrackRecorder::finalisePass")
string(FIND "${multitrack_source_text}" "${multitrack_start_marker}"
       multitrack_start_position)
if (multitrack_start_position EQUAL -1)
    message(FATAL_ERROR "Multitrack callback start marker was not found")
endif()
string(SUBSTRING "${multitrack_source_text}" ${multitrack_start_position} -1
       multitrack_callback_tail)
string(FIND "${multitrack_callback_tail}" "${multitrack_end_marker}"
       multitrack_end_position)
if (multitrack_end_position EQUAL -1)
    message(FATAL_ERROR "Multitrack callback end marker was not found")
endif()
string(SUBSTRING "${multitrack_callback_tail}" 0 ${multitrack_end_position}
       multitrack_callback_text)
foreach (token IN LISTS forbidden_tokens)
    string(FIND "${multitrack_callback_text}" "${token}" found_position)
    if (NOT found_position EQUAL -1)
        message(FATAL_ERROR
            "Multitrack callback contains forbidden operation token: ${token}")
    endif()
endforeach()

message(STATUS "Playback, recording, and multitrack callback lock/allocation audits passed")

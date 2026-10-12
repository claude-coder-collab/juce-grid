include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(JUCE)

add_library(jgrid_juce STATIC)
add_library(jgrid::juce ALIAS jgrid_juce)
target_link_libraries(jgrid_juce
    PRIVATE
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags)
target_compile_definitions(jgrid_juce PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1)
target_include_directories(jgrid_juce SYSTEM INTERFACE
    $<TARGET_PROPERTY:jgrid_juce,INCLUDE_DIRECTORIES>)
target_compile_definitions(jgrid_juce INTERFACE
    $<TARGET_PROPERTY:jgrid_juce,COMPILE_DEFINITIONS>)
set_target_properties(jgrid_juce PROPERTIES CXX_STANDARD 20)
jgrid_enable_sanitizers(jgrid_juce)

if(JGRID_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.1
        GIT_SHALLOW TRUE
        SYSTEM)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    jgrid_enable_sanitizers(Catch2)
    jgrid_enable_sanitizers(Catch2WithMain)
endif()

juce_add_module("${PROJECT_SOURCE_DIR}/modules/jgrid")

juce_add_console_app(jgrid_module_check PRODUCT_NAME "JUCE Grid Module Check")
target_sources(jgrid_module_check PRIVATE Main.cpp)
target_compile_definitions(jgrid_module_check PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(jgrid_module_check PRIVATE jgrid juce::juce_gui_basics juce::juce_recommended_config_flags)
add_test(NAME module_check COMMAND jgrid_module_check)

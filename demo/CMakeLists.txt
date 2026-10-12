juce_add_gui_app(jgrid_demo
    PRODUCT_NAME "JUCE Grid Demo"
    COMPANY_NAME "jgrid"
    BUNDLE_ID "dev.jgrid.demo"
    VERSION ${PROJECT_VERSION})
target_sources(jgrid_demo PRIVATE Main.cpp DemoModel.cpp)
target_link_libraries(jgrid_demo PRIVATE jgrid::jgrid)
target_compile_definitions(jgrid_demo PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:jgrid_demo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:jgrid_demo,JUCE_VERSION>")
jgrid_configure_target(jgrid_demo)

juce_add_console_app(jgrid_examples PRODUCT_NAME "JUCE Grid Examples")
target_sources(jgrid_examples PRIVATE
    01_ReadOnly.cpp
    02_Editable.cpp
    03_Sorting.cpp
    04_RowsAndUndo.cpp
    05_MultipleViews.cpp
    06_MenusAndColours.cpp
    Main.cpp)
target_link_libraries(jgrid_examples PRIVATE jgrid::jgrid)
jgrid_configure_target(jgrid_examples)
add_test(NAME examples COMMAND jgrid_examples)

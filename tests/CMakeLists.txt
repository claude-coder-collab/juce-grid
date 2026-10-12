include(Catch)

add_executable(jgrid_tests
    core/test_Axis.cpp
    core/test_Change.cpp
    core/test_Clipboard.cpp
    core/test_Editing.cpp
    core/test_GridModelNotifier.cpp
    core/test_HeaderLabels.cpp
    core/test_Navigation.cpp
    core/test_RowOrder.cpp
    core/test_Selection.cpp
    core/test_Sorting.cpp
    core/test_ViewModel.cpp
    grid/test_Grid.cpp
    grid/test_GridAccessibility.cpp
    grid/test_GridClipboard.cpp
    grid/test_GridEditing.cpp
    grid/test_GridLayout.cpp
    grid/test_GridNotifications.cpp
    grid/test_GridRows.cpp
    grid/test_GridSelection.cpp
    grid/test_GridSorting.cpp)
target_link_libraries(jgrid_tests PRIVATE jgrid::jgrid Catch2::Catch2WithMain)
jgrid_configure_target(jgrid_tests)
catch_discover_tests(jgrid_tests)

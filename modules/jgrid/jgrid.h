/*
BEGIN_JUCE_MODULE_DECLARATION

    ID:               jgrid
    vendor:           jgrid
    version:          0.1.0
    name:             JUCE Grid
    description:      A spreadsheet-style grid component bound to a user-supplied model.
    minimumCppStandard: 23
    dependencies:     juce_gui_basics

END_JUCE_MODULE_DECLARATION
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/jgrid_Axis.h"
#include "core/jgrid_Change.h"
#include "core/jgrid_Clipboard.h"
#include "core/jgrid_Editing.h"
#include "core/jgrid_GridModel.h"
#include "core/jgrid_GridModelNotifier.h"
#include "core/jgrid_HeaderLabels.h"
#include "core/jgrid_Navigation.h"
#include "core/jgrid_RowOrder.h"
#include "core/jgrid_Selection.h"
#include "core/jgrid_Sorting.h"
#include "core/jgrid_ViewModel.h"
#include "grid/jgrid_Grid.h"

//--------------------------------------------------------------------------------------------------
// DemoMgr.cpp
//
// Handles switching between demos, passing the demos key events, knowing when to exit the app etc.
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

EDemo CDemoMgr::s_currentDemo = e_demoFirst;
bool CDemoMgr::s_exit = false;
float CDemoMgr::s_volumeMultiplier = 1.0f;
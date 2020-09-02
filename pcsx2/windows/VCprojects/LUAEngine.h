#pragma once
#include "Pcsx2Types.h"

enum script_place_type
{
	SPT_ONCE_ON_LOAD = 0,
	SPT_CONTINOUSLY = 1,

	_SPT_END_MARKER
};

extern bool LoadScriptFromDir(wxString name, const wxDirName& folderName, const wxString& friendlyName);
extern void ExecuteScript(script_place_type place);
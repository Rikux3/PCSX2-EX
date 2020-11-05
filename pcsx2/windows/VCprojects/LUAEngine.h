#ifndef LUA_HEAD
#define LUA_HEAD

#include "Pcsx2Types.h"

enum LUAExecutionTime
{
	SPT_ONCE_ON_LOAD = 0,
	SPT_CONTINOUSLY = 1,

	_SPT_END_MARKER
};

extern void ForgetScripts();
extern bool LoadScriptFromDir(wxString Input01, const wxDirName& Input02, const wxString& Input03);
extern void ExecuteScript(LUAExecutionTime Input01);

#endif
#include "PrecompiledHeader.h"
#include "LUAEngine.h"
#include "Memory.h"

#include "sol.hpp"

sol::state LUAEngine;

sol::function ExecuteOnInit;
sol::function ExecuteOnFrame;

namespace
{
	u8 Read01(u32 Input)
	{
		return memRead8(Input);
	}
	u16 Read02(u32 Input)
	{
		return memRead16(Input);
	}
	u32 Read03(u32 Input)
	{
		return memRead32(Input);
	}
	float Read04(u32 Input)
	{
		return (float)memRead32(Input);
	}
	std::vector<u8> Read05(u32 Input01, u32 Input02)
	{
		std::vector<u8> Value(Input02);

		for (size_t i = 0; i < Input02; i++)
		{
			Value.at(i) = memRead8(Input01);
			Input01++;
		}

		return Value;
	}

	u32 Calc01(u32 Input01, u32 Input02)
	{
		u32 Value = memRead32(Input01);
		return Value + Input02;
	}

	void Write01(u32 Input01, u8 Input02)
	{
		if (memRead8(Input01) != (u8)Input02)
			memWrite8(Input01, (u8)Input02);
	}
	void Write02(u32 Input01, u16 Input02)
	{
		if (memRead16(Input01) != (u16)Input02)
			memWrite16(Input01, (u16)Input02);
	}
	void Write03(u32 Input01, u32 Input02)
	{
		if (memRead32(Input01) != (u32)Input02)
			memWrite32(Input01, (u32)Input02);
	}
	void Write04(u32 Input01, float Input02)
	{
		if (memRead32(Input01) != (u32)Input02)
			memWrite32(Input01, (u32)Input02);
	}

	void Misc01(const char* Input, int Color = 1)
	{
		ConsoleColors _color = (ConsoleColors)Color;
		Console.WriteLn(_color, Input);
	}
} // namespace


void ExportFunctionCalls()
{
	// Readers
	LUAEngine.set_function("ReadByte", Read01);
	LUAEngine.set_function("ReadUShort", Read02);
	LUAEngine.set_function("ReadUInt", Read03);
	LUAEngine.set_function("ReadFloat", Read04);
	LUAEngine.set_function("ReadArray", Read05);

	// Calculators
	LUAEngine.set_function("GetPointer", Calc01);

	// Writers
	LUAEngine.set_function("WriteByte", Write01);
	LUAEngine.set_function("WriteUShort", Write02);
	LUAEngine.set_function("WriteUInt", Write03);
	LUAEngine.set_function("WriteFloat", Write04);

	// Misc
	LUAEngine.set_function("Print", Misc01);
}

bool InitScript(wxString path, wxString ScriptTitle)
{
	Console.WriteLn(Color_Green, L"--> Initializing Script: \"%s\"", WX_STR(ScriptTitle));

	LUAEngine.open_libraries
	(
		sol::lib::base,		 
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::io
	);

	ExportFunctionCalls();
	LUAEngine.do_file(path.ToStdString());

	ExecuteOnInit = LUAEngine["_OnInit"];
	ExecuteOnFrame = LUAEngine["_OnFrame"];

	if (!ExecuteOnInit || !ExecuteOnFrame)
	{
		Console.WriteLn(Color_Red, L"--> Not a valid LUAEngine script! Initialization failed!");
		Console.WriteLn(Color_Black, L"");
		return false;
	}

	Console.WriteLn(Color_Green, L"--> Initialization Successful!");
	Console.WriteLn(Color_Black, L"");
	return true;
}

bool LoadScriptFromDir(wxString name, const wxDirName& folderName, const wxString& friendlyName)
{
	Console.WriteLn(Color_Black, L"");
	Console.WriteLn(Color_StrongBlue, L"Initializing LUAEngine v0.37");
	Console.WriteLn(Color_Black, L"");

	if (!folderName.Exists())
	{
		Console.WriteLn(Color_Red, L"--> The \"lua\" folder does not exist! Aborting Initialization...");
		return 0;
	}

	wxString PathString = Path::Combine(folderName, name.MakeUpper() + L".lua");
	wxString LUAName = name.MakeUpper() + L".lua";
	wxFileName FilePath(PathString);

	if (!FilePath.Exists())
	{
		Console.WriteLn(Color_Red, L"--> LUA Script named \"%s\" not found! Aborting...", WX_STR(LUAName));
		return 0;
	}

	Console.WriteLn(Color_Blue, L"--> Initialization Complete!");
	Console.WriteLn(Color_Black, L"");
	return InitScript(PathString, LUAName);
}

void ExecuteScript(script_place_type place)
{
	if (!ExecuteOnInit || !ExecuteOnFrame)
		return;

	switch (place)
	{
		case SPT_ONCE_ON_LOAD:
			ExecuteOnInit();
			break;
		case SPT_CONTINOUSLY:
			ExecuteOnFrame();
			break;
	}
}
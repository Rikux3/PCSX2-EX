#include "PrecompiledHeader.h"
#include "LUAEngine.h"
#include "Memory.h"

#include "sol.hpp"
#include <iostream>
#include <fstream>

using namespace sol;
using namespace std;

state LUAEngine;
sol::function ExecuteOnInit;
sol::function ExecuteOnFrame;

wxString _rootPath;
wxString _loadPath;
wxString _dumpPath;

int _luaVersion = 0x03;

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
	vector<u8> Read05(u32 Input01, u32 Input02)
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

	void File01(u32 Input01, u32 Input02, const char* Input03)
	{
		std::vector<u8> _value(Input02);
		wxString _location = Path::Combine(_dumpPath, Input03);

		for (size_t i = 0; i < Input02; i++)
		{
			_value.at(i) = memRead8(Input01);
			Input01++;
		}

		ofstream _output(_location.ToStdString(), ios::out | ios::binary);

		if (!_output)
		{
			Console.WriteLn(Color_Red, "Unable to open file: " + wxString(Input03));
			return;
		}

		_output.write(reinterpret_cast<char*>(&_value[0]), _value.size() * sizeof(u8));
	}
	void File02(u32 Input01, const char* Input02)
	{
		wxString _location = Path::Combine(_dumpPath, Input02);

		ifstream _input(_location.ToStdString(), ios_base::binary);

		_input.seekg(0, std::ios_base::end);
		size_t _length = _input.tellg();
		_input.seekg(0, std::ios_base::beg);

		vector<char> _buffer;
		_buffer.reserve(_length);
		copy(istreambuf_iterator<char>(_input), istreambuf_iterator<char>(), back_inserter(_buffer));

		for (size_t i = 0; i < _buffer.size(); i++)
		{
			memWrite8(Input01, (u8)_buffer[i]);
			Input01++;
		}
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
	void Write05(u32 Input01, vector<u8> Input02)
	{
		for (size_t i = 0; i < Input02.size(); i++)
		{
			memWrite8(Input01, (u8)Input02[i]);
			Input01++;
		}
	}
	
	void Misc01(const char* Input, int Color = 1)
	{
		ConsoleColors _color = (ConsoleColors)Color;
		Console.WriteLn(_color, Input);
	}
}


void ExportFunctionCalls()
{
	// Readers
	LUAEngine.set_function("ReadByte", Read01);
	LUAEngine.set_function("ReadShort", Read02);
	LUAEngine.set_function("ReadInt", Read03);
	LUAEngine.set_function("ReadFloat", Read04);
	LUAEngine.set_function("ReadArray", Read05);

	// Calculators
	LUAEngine.set_function("GetPointer", Calc01);

	// IO Operations
	LUAEngine.set_function("CreateDump", File01);
	LUAEngine.set_function("WriteFile", File02);

	// Writers
	LUAEngine.set_function("WriteByte", Write01);
	LUAEngine.set_function("WriteShort", Write02);
	LUAEngine.set_function("WriteInt", Write03);
	LUAEngine.set_function("WriteFloat", Write04);
	LUAEngine.set_function("WriteArray", Write05);

	// Misc
	LUAEngine.set_function("Print", Misc01);
}

bool InitScript(wxString path, wxString ScriptTitle)
{
	Console.WriteLn(Color_Green, L"--> Initializing Script: \"%s\"", WX_STR(ScriptTitle));

	LUAEngine.open_libraries
	(
		lib::base,		 
		lib::package,
		lib::coroutine,
		lib::string,
		lib::os,
		lib::math,
		lib::table,
		lib::io
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

	_loadPath = Path::Combine(Path::GetDirectory(path),  "io_load");
	_dumpPath = Path::Combine(Path::GetDirectory(path), "io_dump");

	Console.WriteLn(Color_Black, _loadPath);
	Console.WriteLn(Color_Black, _dumpPath);

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
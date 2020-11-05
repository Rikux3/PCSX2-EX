#include "PrecompiledHeader.h"
#include "Memory.h"
#include "LUAEngine.h"

#include <iostream>
#include <fstream>

#include <memory>
#include <vector>
#include <wx/textfile.h>
#include <wx/dir.h>
#include <wx/txtstrm.h>
#include <wx/zipstrm.h>

#include "sol.hpp"


using namespace sol;
using namespace std;

class LUAScript
{
	static int _version;
	static wxString _checksum;
	static wxString _loadPath;
	static wxString _savePath;

public:
	state luaState;
	sol::function initFunction;
	sol::function frameFunction;

	static u8 Read01(u32 Input)
	{
		return memRead8(Input);
	}
	static u16 Read02(u32 Input)
	{
		return memRead16(Input);
	}
	static u32 Read03(u32 Input)
	{
		return memRead32(Input);
	}
	static float Read04(u32 Input)
	{
		u32 _readMem = memRead32(Input);
		return *reinterpret_cast<float*>(&_readMem);
	}
	static vector<u8> Read05(u32 Input01, u32 Input02)
	{
		std::vector<u8> Value(Input02);

		for (size_t i = 0; i < Input02; i++)
		{
			Value.at(i) = memRead8(Input01);
			Input01++;
		}

		return Value;
	}
	static string Read06(u32 Input01, u32 Input02)
	{
		std::vector<u8> Value(Input02);

		for (size_t i = 0; i < Input02; i++)
		{
			Value.at(i) = memRead8(Input01);
			Input01++;
		}

		return string(Value.begin(), Value.end());
	}

	static u32 Calc01(u32 Input01, u32 Input02)
	{
		u32 Value = memRead32(Input01);
		return Value + Input02;
	}

	static void File01(u32 Input01, u32 Input02, const char* Input03)
	{
		std::vector<u8> _value(Input02);
		wxString _location = Path::Combine(_savePath, Input03);

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
	static vector<u8> File02(const char* Input02)
	{
		wxString _location = Path::Combine(_loadPath, Input02);

		ifstream _input(_location.ToStdString(), ios_base::binary);

		_input.seekg(0, std::ios_base::end);
		size_t _length = _input.tellg();
		_input.seekg(0, std::ios_base::beg);

		vector<u8> _buffer;
		_buffer.reserve(_length);
		copy(istreambuf_iterator<char>(_input), istreambuf_iterator<char>(), back_inserter(_buffer));

		return _buffer;
	}

	static void Write01(u32 Input01, u8 Input02)
	{
		if (memRead8(Input01) != (u8)Input02)
			memWrite8(Input01, (u8)Input02);
	}
	static void Write02(u32 Input01, u16 Input02)
	{
		if (memRead16(Input01) != (u16)Input02)
			memWrite16(Input01, (u16)Input02);
	}
	static void Write03(u32 Input01, u32 Input02)
	{
		if (memRead32(Input01) != (u32)Input02)
			memWrite32(Input01, (u32)Input02);
	}
	static void Write04(u32 Input01, float Input02)
	{
		float _inp = Input02;
		u32 _val = *reinterpret_cast<u32*>(&_inp);

		if (memRead32(Input01) != (u32)_val)
			memWrite32(Input01, (u32)_val);
	}
	static void Write05(u32 Input01, vector<u8> Input02)
	{
		for (size_t i = 0; i < Input02.size(); i++)
		{
			memWrite8(Input01, (u8)Input02[i]);
			Input01++;
		}
	}
	static void Write06(u32 Input01, string Input02)
	{
		vector<uint8_t> Value(Input02.begin(), Input02.end());

		for (size_t i = 0; i < Value.size(); i++)
		{
			memWrite8(Input01, (u8)Value[i]);
			Input01++;
		}
	}

	static void Misc01(const char* Input, int Color = 1)
	{
		ConsoleColors _color = (ConsoleColors)Color;
		Console.WriteLn(_color, Input);
	}
	static int Misc02()
	{
		return _version;
	}
	static string Misc03()
	{
		return _checksum.Upper().ToStdString();
	}

	void SetFunctions();

	LUAScript(wxString Input01, wxString Input02);
};

int LUAScript::_version;
wxString LUAScript::_checksum;
wxString LUAScript::_loadPath;
wxString LUAScript::_savePath;

vector<wxString> _scriptNames;
vector<LUAScript*> _loadedScripts;

void LUAScript::SetFunctions()
{
		// Readers
		luaState.set_function("ReadByte", Read01);
		luaState.set_function("ReadShort", Read02);
		luaState.set_function("ReadInt", Read03);
		luaState.set_function("ReadFloat", Read04);
		luaState.set_function("ReadArray", Read05);
		luaState.set_function("ReadString", Read06);

		// Calculators
		luaState.set_function("GetPointer", Calc01);

		// IO Operations
		luaState.set_function("CreateDump", File01);
		luaState.set_function("ReadFile", File02);

		// Writers
		luaState.set_function("WriteByte", Write01);
		luaState.set_function("WriteShort", Write02);
		luaState.set_function("WriteInt", Write03);
		luaState.set_function("WriteFloat", Write04);
		luaState.set_function("WriteArray", Write05);
		luaState.set_function("WriteString", Write06);


		// Misc
		luaState.set_function("Print", Misc01);
		luaState.set_function("GetVersion", Misc02);
		luaState.set_function("GetGameCode", Misc03);
	}

LUAScript::LUAScript(wxString Input01, wxString Input02)
{
	luaState.open_libraries
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

	SetFunctions();

	_version = 0x0507;
	luaState.do_file(Input01.ToStdString());

	initFunction = luaState["_OnInit"];
	frameFunction = luaState["_OnFrame"];

	if (!initFunction)
		Console.WriteLn(Color_Red, L"LUAEngine: The \"_OnInit\" function either has errors or does not exist.\n");

	if (!frameFunction)
		Console.WriteLn(Color_Red, L"LUAEngine: The \"_OnFrame\" function either has errors or does not exist.\n");

	if (!initFunction && !frameFunction)
	{
		Console.WriteLn(Color_Red, L"LUAEngine: Both the \"_OnInit\" and \"_OnFrame\" functions either have errors or do not exist!\n");
		Console.WriteLn(Color_Red, L"LUAEngine: Initialization of this script cannot continue...");
		return;
	}

	_loadPath = Path::Combine(Path::GetDirectory(Input01), "io_load");
	_savePath = Path::Combine(Path::GetDirectory(Input01), "io_save");

	_checksum = Input02;

	Console.WriteLn(Color_Green, L"LUAEngine: Initialization Successful!");
	Console.WriteLn(Color_Black, L"");
}

void ForgetScripts()
{
	_scriptNames.clear();
	_loadedScripts.clear();
}
bool LoadScriptFromDir(wxString name, const wxDirName& folderName, const wxString& friendlyName)
{
	Console.WriteLn(Color_Black, L"");
	Console.WriteLn(Color_StrongBlue, L"Initializing LUAEngine v1.25");
	Console.WriteLn(Color_Black, L"");

	if (!folderName.Exists())
	{
		Console.WriteLn(Color_Red, L"LUAEngine: The \"lua\" folder does not exist! Aborting Initialization...");
		return false;
	}

	int _interval = 0;

	wxString _buffer;
	wxString _luaName = "*" + name + L"*.lua";
	wxDir _dirName(folderName.ToString());

	bool _found = _dirName.GetFirst(&_buffer, L"*", wxDIR_FILES);

	while (_found) 
	{
		if (_buffer.Upper().Matches(_luaName.Upper())) 
		{
			try
			{
				if (_buffer == _scriptNames.at(_interval)) 
				{
					_interval++;
					_found = _dirName.GetNext(&_buffer);
					continue;
				}

				else
				{
					Console.WriteLn(Color_Green, L"LUAEngine: Found LUA Script: '%s'! Initializing...", WX_STR(_buffer));
					_scriptNames.push_back(_buffer);
					_loadedScripts.push_back(new LUAScript(Path::Combine(_dirName.GetName(), _buffer), name));
				}
			}
			
			catch (const out_of_range& e)
			{
				Console.WriteLn(Color_Green, L"LUAEngine: Found LUA Script: '%s'! Initializing...", WX_STR(_buffer));
				_scriptNames.push_back(_buffer);
				_loadedScripts.push_back(new LUAScript(Path::Combine(_dirName.GetName(), _buffer), name));
			}

			_interval++;
		}

		_found = _dirName.GetNext(&_buffer);
	}
}
void ExecuteScript(LUAExecutionTime Input01)
{
	for each (auto _script in _loadedScripts)
	{
		if (!_script->initFunction && !_script->frameFunction)
			return;

		switch (Input01)
		{
			case SPT_ONCE_ON_LOAD:
				_script->initFunction();
				break;
			case SPT_CONTINOUSLY:
				_script->frameFunction();
				break;
		}
	}
}
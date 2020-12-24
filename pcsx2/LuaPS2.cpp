#include "PrecompiledHeader.h"
#include "LuaPS2.h"

int LuaPS2::_version;
u32 LuaPS2::_checksum;
wxString LuaPS2::_loadPath;
wxString LuaPS2::_savePath;

LuaPS2::LuaPS2(wxString Input01, u32 Input02)
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

	const string _packagePath = luaState["package"]["path"];
	luaState["package"]["path"] = Path::Combine(Path::GetDirectory(Input01), "io_packages/").ToStdString() + "?.lua";

	_version = 0x0600;
	luaState.do_file(Input01.ToStdString());

	initFunction = luaState["_OnInit"];
	frameFunction = luaState["_OnFrame"];

	if (!initFunction)
		Console.WriteLn(Color_Red, L"LuaEngine: The \"_OnInit\" function either has errors or does not exist.");

	if (!frameFunction)
		Console.WriteLn(Color_Red, L"LuaEngine: The \"_OnFrame\" function either has errors or does not exist.");

	if (!initFunction && !frameFunction)
	{
		Console.WriteLn(Color_Red, L"\nLuaEngine: Both the \"_OnInit\" and \"_OnFrame\" functions either have errors or do not exist!");
		Console.WriteLn(Color_Red, L"LuaEngine: Initialization of this script cannot continue...\n");
		return;
	}

	_loadPath = Path::Combine(Path::GetDirectory(Input01), "io_load");
	_savePath = Path::Combine(Path::GetDirectory(Input01), "io_save");

	_checksum = Input02;

	Console.WriteLn(Color_Green, L"LuaEngine: Initialization Successful!");
	Console.WriteLn(Color_Black, L"");
}


void LuaPS2::SetFunctions()
{
	// Readers
	luaState.set_function("ReadByte",    Read_UInt08);
	luaState.set_function("ReadShort",   Read_UInt16);
	luaState.set_function("ReadInt",     Read_UInt32);
	luaState.set_function("ReadLong",    Read_UInt64);
	luaState.set_function("ReadBoolean", Read_Boolean);
	luaState.set_function("ReadFloat",   Read_Single);
	luaState.set_function("ReadString",  Read_String);
	luaState.set_function("ReadArray",   Read_UInt08_Array);

	// Calculators
	luaState.set_function("GetPointer", Calculate_Pointer);

	// IO Operations
	luaState.set_function("MakeDump", File_DumpRAM);
	luaState.set_function("ReadFile", sol::overload(File_Read, File_ReadRegion));

	// Writers
	luaState.set_function("WriteByte",    Write_UInt08);
	luaState.set_function("WriteShort",   Write_UInt16);
	luaState.set_function("WriteInt",     Write_UInt32);
	luaState.set_function("WriteLong",    Write_UInt64);
	luaState.set_function("WriteBoolean", Write_Boolean);
	luaState.set_function("WriteFloat",   Write_Single);
	luaState.set_function("WriteString",  Write_String);
	luaState.set_function("WriteArray",   Write_UInt08_Array);

	// Misc
	luaState.set_function("ConsolePrint", PCSX2Print);
	luaState.set_function("LuaVersion", FetchVersion);
	luaState.set_function("GameChecksum", FetchChecksum);
}


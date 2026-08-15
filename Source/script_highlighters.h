#pragma once

/*-------------------------------------------------------------+
|                                                              |
|                   _________   ______ _    _____              |
|                  / / ____/ | / / __ \ |  / /   |             |
|             __  / / __/ /  |/ / / / / | / / /| |             |
|            / /_/ / /___/ /|  / /_/ /| |/ / ___ |             |
|            \____/_____/_/ |_/\____/ |___/_/  |_|             |
|                                                              |
|                        Jenova Runtime                        |
|                   Developed by Hamid.Memar                   |
|                                                              |
+-------------------------------------------------------------*/

// Jenova SDK
#include "Jenova.hpp""

// Jenova Editor Syntax Highlighter Definition
class CPPSyntaxHighlighter : public EditorSyntaxHighlighter
{
	GDCLASS(CPPSyntaxHighlighter, EditorSyntaxHighlighter)

private:
	// Internal Values
	mutable Vector<Dictionary> lexerCache;
	mutable HashMap<int, int> colorRegionCache;
	mutable int64_t lastCacheHash = 0;
	mutable bool signalConnected = false;

	// Database
	PackedStringArray cppTypes;
	PackedStringArray cppKeywords;
	PackedStringArray cppControlFlow;
	PackedStringArray specialKeywords;
	PackedStringArray macroKeywords;
	bool wordDatabasesInitialized = false;

public:
	// Define C++ Highlighter Colors
	Color cpp_typeColor					= Color::html("#aa97fc");
	Color cpp_keywordColor				= Color::html("#6e9ffa");
	Color cpp_controlFlowColor			= Color::html("#7cf0ff");
	Color cpp_classColor				= Color::html("#fcc762");
	Color cpp_booleanColor				= Color::html("#3c6fe6");
	Color cpp_preprocessorColor			= Color::html("#f0ac81");
	Color cpp_stringColor				= Color::html("#ff3766");
	Color cpp_functionColor				= Color::html("#a3ff9b");
	Color cpp_numberColor				= Color::html("#fff188");
	Color cpp_pointerColor				= Color::html("#c4ff6b");
	Color cpp_commentColor				= Color::html("#a6a6a6aa");
	Color cpp_operatorColor				= Color::html("#aefce3");
	Color cpp_specialColor				= Color::html("#47ffaf");
	Color cpp_macroColor				= Color::html("#ffb8b8");

protected:
	static void _bind_methods() {}

public:
	static void init();
	static void deinit();

	virtual String _get_name() const override;
	virtual PackedStringArray _get_supported_languages() const override;
	virtual Ref<EditorSyntaxHighlighter> _create() const override;
	virtual Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
	void _on_lines_edited(int p_from_line, int p_to_line);
	virtual void _clear_highlighting_cache() override;
	virtual void _update_cache() override;

public:
	// Initializer
	CPPSyntaxHighlighter()
	{
		// Initialize Databases
		cppTypes =
		{
			"bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "int8_t", "uint8_t", "int16_t", "int32_t", "int64_t", "uint16_t", "uint32_t", "uint64_t",
			"long", "size_t", "short", "signed", "unsigned", "void", "wchar_t", "__int64", "__int32", "nullptr_t", "ptrdiff_t", "max_align_t", "byte", "json_t", "string",
			"vector", "array", "map", "set", "unordered_map", "unordered_set", "list", "deque", "queue", "stack", "pair", "tuple", "optional", "variant", "any", "unique_ptr",
			"shared_ptr", "weak_ptr", "NIL", "String", "Vector2", "Vector2i", "Rect2", "Rect2i", "Vector3", "Vector3i", "Transform2D", "Vector4", "Vector4i",
			"Plane", "Quaternion", "AABB", "Basis", "Transform3D", "Projection", "Color", "StringName", "NodePath", "RID", "Object", "Callable", "Signal", "Dictionary", "Array",
			"PackedByteArray", "PackedInt32Array", "PackedInt64Array", "PackedFloat32Array", "PackedFloat64Array", "PackedStringArray", "PackedVector2Array", "PackedVector3Array",
			"PackedColorArray", "PackedVector4Array"
		};
		cppKeywords =
		{
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "class", "compl", "concept",
			"const", "consteval", "constexpr", "const_cast", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "new", "dynamic_cast", "enum", "explicit",
			"export", "extern", "false", "friend", "inline", "mutable", "namespace", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
			"public", "reflexpr", "register", "reinterpret_cast", "requires", "sizeof", "static", "static_assert", "static_cast", "struct", "synchronized", "template", "this",
			"thread_local", "throw", "true", "typedef", "typeid", "typename", "union", "using", "virtual", "volatile", "xor", "xor_eq", "include", "ifdef", "endif", "pragma",
			"final", "override"
		};
		cppControlFlow =
		{
			"break", "case", "catch", "continue", "default", "do", "else",
			"for", "goto", "if", "return", "switch", "throw", "try", "while",
			"co_await", "co_return", "co_yield"
		};
		specialKeywords =
		{
			"jenova", "sdk", "godot", "std", "Ref",
			"Caller", "Variant", "CarbonScript", "JenovaSDK",
			"EngineMode", "FileSystemEvent", "RuntimeReloadMode", "RuntimeEvent", "ClassAccess",
			"FunctionPtr", "NativePtr", "IntPtr", "BufferPtr", "ObjectPtr", "StringPtr", "WideStringPtr",
			"ImageSize", "MemoryID", "VariableID", "TaskID", "UniqueID", "DriverResourceID", "TaskFunction",
			"FutureFunction", "JenovaSDKInterface", "GetSelf", "GetNode", "FindNode", "GlobalPointer",
			"GlobalGet", "GlobalSet", "GlobalVariable", "GetObjectFromIntPtr", "Instantiate", "InstantiateAsRef"
		};
		macroKeywords =
		{
			jenova::GlobalSettings::ScriptToolIdentifier,
			jenova::GlobalSettings::ScriptCarbonIdentifier,
			jenova::GlobalSettings::ScriptRecordIdentifier,
			jenova::GlobalSettings::ScriptBlockBeginIdentifier,
			jenova::GlobalSettings::ScriptBlockEndIdentifier,
			jenova::GlobalSettings::ScriptVMBeginIdentifier,
			jenova::GlobalSettings::ScriptVMEndIdentifier,
			jenova::GlobalSettings::ScriptIDIdentifier,
			jenova::GlobalSettings::ScriptSignalCallbackIdentifier,
			jenova::GlobalSettings::ScriptPropertyIdentifier,
			jenova::GlobalSettings::ScriptSignalIdentifier,
			jenova::GlobalSettings::ScriptClassNameIdentifier,
			jenova::GlobalSettings::ScriptActivatorIdentifier,
			jenova::GlobalSettings::ScriptFunctionExportIdentifier
		};
	}

private:
	// Internal Methods
	Dictionary HighlightLine(int32_t p_line) const;
	void PerformHighlighting() const;
	void ResetHighlighting() const;
};
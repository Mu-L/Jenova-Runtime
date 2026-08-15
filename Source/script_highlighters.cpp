
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
#include "Jenova.hpp"

// Minimal C++ Lexer
#include "Lexers/Minilex.hpp"

// Initializer/Deinitializer
void CPPSyntaxHighlighter::init()
{
	// Register Class
	ClassDB::register_internal_class<CPPSyntaxHighlighter>();

	// Register Syntax Highlighter
	Ref<CPPSyntaxHighlighter> highlighter; highlighter.instantiate();
	EditorInterface::get_singleton()->get_script_editor()->register_syntax_highlighter(highlighter);
}
void CPPSyntaxHighlighter::deinit()
{
	// Unregister Syntax Highlighter
	Ref<CPPSyntaxHighlighter> highlighter; highlighter.instantiate();
	EditorInterface::get_singleton()->get_script_editor()->unregister_syntax_highlighter(highlighter);
}

// Jenova Editor Syntax Highlighter Implementation
String CPPSyntaxHighlighter::_get_name() const
{
	return CPPScriptLanguage::get_singleton()->_get_friendly_name();
}
PackedStringArray CPPSyntaxHighlighter::_get_supported_languages() const
{
	return PackedStringArray{ CPPScriptLanguage::get_singleton()->_get_name(), CPPHeaderLanguage::get_singleton()->_get_name() };
}
Ref<EditorSyntaxHighlighter> CPPSyntaxHighlighter::_create() const
{
	Ref<CPPSyntaxHighlighter> highlighter;
	highlighter.instantiate();
	return highlighter;
}
Dictionary CPPSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const
{
	TextEdit* textEdit = get_text_edit();
	if (textEdit)
	{
		int lineCount = textEdit->get_line_count();
		if (lexerCache.size() != lineCount) lexerCache.resize(lineCount);
	}
	if (p_line >= 0 && p_line < lexerCache.size() && !lexerCache[p_line].is_empty()) return lexerCache[p_line];
	return HighlightLine(p_line);
}
void CPPSyntaxHighlighter::_on_lines_edited(int p_from_line, int p_to_line)
{
	// Determine Starting Line for Invalidation
	int startLine = MIN(p_from_line, p_to_line) - 1;
	if (startLine < 0) startLine = 0;

	// Clear Lexer Cache from Start Line Onward
	for (int i = startLine; i < lexerCache.size(); i++) lexerCache.set(i, Dictionary());

	// Rebuild Region Cache from Scratch
	colorRegionCache.clear();

	TextEdit* textEdit = get_text_edit();
	if (textEdit)
	{
		// Build Region State Up to Start Line
		int regionState = -1;
		for (int i = 0; i < startLine && i < textEdit->get_line_count(); i++)
		{
			String line = textEdit->get_line(i);
			if (regionState == 0)
			{
				// Currently In Comment
				if (line.find("*/") != -1)
				{
					regionState = -1;
					colorRegionCache.insert(i, regionState);
				}
				else
				{
					colorRegionCache.insert(i, 0);
				}
			}
			else
			{
				// Not In Comment
				if (line.find("/*") != -1 && line.find("*/") == -1)
				{
					regionState = 0;
					colorRegionCache.insert(i, 0);
				}
				else
				{
					colorRegionCache.insert(i, -1);
				}
			}
		}
	}

	lastCacheHash = 0;
}
void CPPSyntaxHighlighter::_update_cache()
{
	TextEdit* textEdit = get_text_edit();
	if (textEdit)
	{
		// Disconnect Old Connection If Any
		if (signalConnected)
		{
			textEdit->disconnect("lines_edited_from", callable_mp(this, &CPPSyntaxHighlighter::_on_lines_edited));
			signalConnected = false;
		}

		// Connect Signals for Edit Detection
		textEdit->connect("lines_edited_from", callable_mp(this, &CPPSyntaxHighlighter::_on_lines_edited));
		signalConnected = true;
	}

	PerformHighlighting();
}
void CPPSyntaxHighlighter::_clear_highlighting_cache()
{
	ResetHighlighting();
}
Dictionary CPPSyntaxHighlighter::HighlightLine(int32_t p_line) const
{
	TextEdit* textEdit = get_text_edit();
	if (!textEdit) return Dictionary();

	int lineCount = textEdit->get_line_count();
	if (lexerCache.size() != lineCount) lexerCache.resize(lineCount);
	if (p_line < 0 || p_line >= lexerCache.size()) return Dictionary();

	String line = textEdit->get_line(p_line);
	Dictionary colorMap;

	if (line.length() == 0)
	{
		lexerCache.set(p_line, colorMap);
		return colorMap;
	}

	int regionState = -1;
	if (p_line > 0)
	{
		int prevLine = p_line - 1;
		while (prevLine > 0 && !colorRegionCache.has(prevLine)) prevLine--;
		if (colorRegionCache.has(prevLine)) regionState = colorRegionCache[prevLine];
	}

	// Prepare for Parse
	Color defaultColor = textEdit->get_theme_color("font_color", "TextEdit");
	Color previousColor = defaultColor;

	// Helpers
	auto processIdentifier = [&](const String& word, int tokenStart, int tokenLength, const std::string& currentLine, Color& currentColor) -> void
	{
		if (cppControlFlow.has(word)) currentColor = cpp_controlFlowColor;
		else if (cppKeywords.has(word))
		{
			if (word == "true" || word == "false") currentColor = cpp_booleanColor;
			else if (word == "nullptr") currentColor = cpp_pointerColor;
			else currentColor = cpp_keywordColor;
		}
		else if (cppTypes.has(word)) currentColor = cpp_typeColor;
		else if (specialKeywords.has(word)) currentColor = cpp_specialColor;
		else if (macroKeywords.has(word)) currentColor = cpp_macroColor;
		else if (ClassDB::class_exists(word)) currentColor = cpp_classColor;
		else
		{
			int lookAhead = tokenStart + tokenLength;
			while (lookAhead < int(currentLine.length()) && is_space(currentLine[lookAhead])) lookAhead++;
			if (lookAhead < int(currentLine.length()) && currentLine[lookAhead] == '(') currentColor = cpp_functionColor;
			else currentColor = defaultColor;
		}
	};

	// Handle Multi-Line Comment
	if (regionState == 0)
	{
		int commentEnd = line.find("*/");
		if (commentEnd != -1)
		{
			Dictionary infoPrimary;
			infoPrimary["color"] = cpp_commentColor;
			colorMap[0] = infoPrimary;
			regionState = -1;

			// Handle Remaining Line
			String remainingLine = line.substr(commentEnd + 2);
			if (remainingLine.length() > 0)
			{
				std::string cppRemaining = AS_STD_STRING(remainingLine);
				Lexer lexer(cppRemaining.c_str());
				Token token = lexer.next();
				Color previousCommentColor = cpp_commentColor;

				// Handle Tokens
				while (token.kind() != Token::Kind::End)
				{
					Color currentColor = defaultColor;
					int tokenStart = token.lexeme().data() - cppRemaining.c_str();
					int adjustedStart = commentEnd + 2 + tokenStart;

					switch (token.kind())
					{
					case Token::Kind::Comment:
						currentColor = cpp_commentColor;
						break;
					case Token::Kind::DoubleQuote:
					case Token::Kind::SingleQuote:
						currentColor = cpp_stringColor;
						break;
					case Token::Kind::Number:
						currentColor = cpp_numberColor;
						break;
					case Token::Kind::Identifier:
						processIdentifier(String(std::string(token.lexeme()).c_str()), tokenStart, token.lexeme().length(), cppRemaining, currentColor);
						break;
					case Token::Kind::Asterisk:
						currentColor = cpp_pointerColor;
						break;
					default:
						currentColor = cpp_operatorColor;
						break;
					}

					// Apply Color If Changed
					if (currentColor != previousCommentColor)
					{
						Dictionary infoSecondary;
						infoSecondary["color"] = currentColor;
						colorMap[adjustedStart] = infoSecondary;
						previousCommentColor = currentColor;
					}
					token = lexer.next();
				}
			}
		}
		else
		{
			// It's Fully Comment
			Dictionary info;
			info["color"] = cpp_commentColor;
			colorMap[0] = info;
			regionState = 0;
		}

		colorRegionCache.insert(p_line, regionState);
		lexerCache.set(p_line, colorMap);
		return colorMap;
	}

	// Prepare Lexer
	std::string cppLine = AS_STD_STRING(line);
	Lexer lexer(cppLine.c_str());
	Token token = lexer.next();

	// Handle Tokens
	while (token.kind() != Token::Kind::End)
	{
		Color currentColor = defaultColor;
		int tokenStart = token.lexeme().data() - cppLine.c_str();
		int tokenLength = token.lexeme().length();

		if (token.kind() == Token::Kind::Hash)
		{
			currentColor = cpp_preprocessorColor;
			Dictionary infoPrimary;
			infoPrimary["color"] = currentColor;
			colorMap[tokenStart] = infoPrimary;
			previousColor = currentColor;
			int lookAhead = tokenStart + tokenLength;
			while (lookAhead < int(cppLine.length()))
			{
				if (cppLine[lookAhead] == '\n' || cppLine[lookAhead] == '\0') break;
				if (is_identifier_char(cppLine[lookAhead]))
				{
					int wordStart = lookAhead;
					while (lookAhead < int(cppLine.length()) && is_identifier_char(cppLine[lookAhead])) lookAhead++;

					Dictionary infoSecondary;
					infoSecondary["color"] = cpp_preprocessorColor;
					colorMap[wordStart] = infoSecondary;
					previousColor = cpp_preprocessorColor;
				}
				else lookAhead++;
			}
			while (token.kind() != Token::Kind::End) token = lexer.next();
			continue;
		}

		switch (token.kind())
		{
		case Token::Kind::Comment:
		{
			currentColor = cpp_commentColor;
			std::string commentText = std::string(token.lexeme());
			if (commentText.find("/*") != std::string::npos && commentText.find("*/") == std::string::npos) regionState = 0;
			break;
		}
		case Token::Kind::DoubleQuote:
		case Token::Kind::SingleQuote:
			currentColor = cpp_stringColor;
			break;
		case Token::Kind::Number:
			currentColor = cpp_numberColor;
			break;
		case Token::Kind::Identifier:
			processIdentifier(String(std::string(token.lexeme()).c_str()), tokenStart, tokenLength, cppLine, currentColor);
			break;
		case Token::Kind::Asterisk:
			currentColor = cpp_pointerColor;
			break;
		case Token::Kind::Minus:
			if (token.lexeme().length() > 1 && token.lexeme()[1] == '>') currentColor = cpp_operatorColor;
			else currentColor = cpp_operatorColor;
			break;
		default:
			currentColor = cpp_operatorColor;
			break;
		}

		// Apply Color If Changed
		if (currentColor != previousColor)
		{
			Dictionary info;
			info["color"] = currentColor;
			colorMap[tokenStart] = info;
			previousColor = currentColor;
		}

		token = lexer.next();
	}

	colorRegionCache.insert(p_line, regionState);
	lexerCache.set(p_line, colorMap);
	return colorMap;
}
void CPPSyntaxHighlighter::PerformHighlighting() const
{
	// Get Script Text Editor
	TextEdit* textEdit = get_text_edit();
	if (!textEdit) return;

	// Check For Content Changes
	int64_t contentHash = textEdit->get_text().hash();
	if (contentHash == lastCacheHash) return;
	lastCacheHash = contentHash;

	// Clear Cache
	ResetHighlighting();

	// Prepare For Parse
	int lineCount = textEdit->get_line_count();
	lexerCache.resize(lineCount);

	// Get Visible Lines
	int firstVisible = textEdit->get_first_visible_line();
	int lastVisible = textEdit->get_last_full_visible_line();

	// Process Visible Lines
	for (int lineNum = firstVisible; lineNum <= lastVisible && lineNum < lineCount; lineNum++) HighlightLine(lineNum);
}
void CPPSyntaxHighlighter::ResetHighlighting() const
{
	lexerCache.clear();
	colorRegionCache.clear();
	lastCacheHash = 0;
}
/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

class CFont
{
public:
	char m_aFilename[512];
};

class CTextRender : public IEngineTextRender
{
public:
	CTextRender()
	{
	}

	virtual void Init()
	{
	}


	virtual CFont *LoadFont(const char *pFilename)
	{
		CFont *pFont = (CFont *)mem_alloc(sizeof(CFont), 1);

		mem_zero(pFont, sizeof(*pFont));
		str_copy(pFont->m_aFilename, pFilename, sizeof(pFont->m_aFilename));

		return pFont;
	};

	virtual void DestroyFont(CFont *pFont)
	{
		mem_free(pFont);
	}

	virtual void SetDefaultFont(CFont *pFont)
	{
	}

	virtual void SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags)
	{
		mem_zero(pCursor, sizeof(*pCursor));
		pCursor->m_FontSize = 1;
		pCursor->m_StartX = 2;
		pCursor->m_StartY = 2;
		pCursor->m_X = 3;
		pCursor->m_Y = 4;
		pCursor->m_LineCount = 1;
		pCursor->m_LineWidth = -1;
		pCursor->m_Flags = Flags;
		pCursor->m_CharCount = 0;
	}


	virtual void Text(void *pFontSetV, float x, float y, float Size, const char *pText, int MaxWidth)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, x, y, Size, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = MaxWidth;
	}

	virtual float TextWidth(void *pFontSetV, float Size, const char *pText, int Length)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, 0, 0, Size, 0);
		return Cursor.m_X;
	}

	virtual int TextLineCount(void *pFontSetV, float Size, const char *pText, float LineWidth)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, 0, 0, Size, 0);
		Cursor.m_LineWidth = LineWidth;
		return Cursor.m_LineCount;
	}

	virtual void TextColor(float r, float g, float b, float a)
	{
	}

	virtual void TextOutlineColor(float r, float g, float b, float a)
	{
	}

	virtual void TextEx(CTextCursor *pCursor, const char *pText, int Length)
	{
	}
};

IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }

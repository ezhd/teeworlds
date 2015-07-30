/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/detect.h>
#include <base/math.h>
#include <base/tl/threading.h>

#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <math.h> // cosf, sinf

#include "graphics.h"

CGraphics_OpenGL::CGraphics_OpenGL()
{
}

void CGraphics_OpenGL::ClipEnable(int x, int y, int w, int h)
{
}

void CGraphics_OpenGL::ClipDisable()
{
}

void CGraphics_OpenGL::BlendNone()
{
}

void CGraphics_OpenGL::BlendNormal()
{
}

void CGraphics_OpenGL::BlendAdditive()
{
}

void CGraphics_OpenGL::WrapNormal()
{
}

void CGraphics_OpenGL::WrapClamp()
{
}

int CGraphics_OpenGL::MemoryUsage() const
{
	return 10000000;
}

void CGraphics_OpenGL::MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY)
{
}

void CGraphics_OpenGL::GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY)
{
	*pTopLeftX = 0;
	*pTopLeftY = 0;
	*pBottomRightX = 640;
	*pBottomRightY = 480;
}

void CGraphics_OpenGL::LinesBegin()
{
}

void CGraphics_OpenGL::LinesEnd()
{
}

void CGraphics_OpenGL::LinesDraw(const CLineItem *pArray, int Num)
{
}

int CGraphics_OpenGL::UnloadTexture(int Index)
{
	return 0;
}

int CGraphics_OpenGL::LoadTextureRawSub(int TextureID, int x, int y, int Width, int Height, int Format, const void *pData)
{
	return 0;
}

int CGraphics_OpenGL::LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags)
{
	return 1;
}

// simple uncompressed RGBA loaders
int CGraphics_OpenGL::LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags)
{
	return 1;
}

int CGraphics_OpenGL::LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	class IStorage *m_pStorage = Kernel()->RequestInterface<IStorage>();

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", pFilename);
		return 0;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return 0;
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return 0;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return 1;
}

void CGraphics_OpenGL::ScreenshotDirect(const char *pFilename)
{
}

void CGraphics_OpenGL::TextureSet(int TextureID)
{
}

void CGraphics_OpenGL::Clear(float r, float g, float b)
{
}

void CGraphics_OpenGL::QuadsBegin()
{
}

void CGraphics_OpenGL::QuadsEnd()
{
}

void CGraphics_OpenGL::QuadsSetRotation(float Angle)
{
}

void CGraphics_OpenGL::SetColorVertex(const CColorVertex *pArray, int Num)
{
}

void CGraphics_OpenGL::SetColor(float r, float g, float b, float a)
{
}

void CGraphics_OpenGL::QuadsSetSubset(float TlU, float TlV, float BrU, float BrV)
{
}

void CGraphics_OpenGL::QuadsSetSubsetFree(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
}

void CGraphics_OpenGL::QuadsDraw(CQuadItem *pArray, int Num)
{
}

void CGraphics_OpenGL::QuadsDrawTL(const CQuadItem *pArray, int Num)
{
}

void CGraphics_OpenGL::QuadsDrawFreeform(const CFreeformItem *pArray, int Num)
{
}

void CGraphics_OpenGL::QuadsText(float x, float y, float Size, const char *pText)
{
}

int CGraphics_OpenGL::Init()
{
	return 0;
}

int CGraphics_SDL::TryInit()
{
	g_Config.m_GfxScreenWidth = 640;
	g_Config.m_GfxScreenHeight = 480;
	return 0;
}


int CGraphics_SDL::InitWindow()
{
	return 0;
}


CGraphics_SDL::CGraphics_SDL()
{
}

int CGraphics_SDL::Init()
{
	return 0;
}

void CGraphics_SDL::Shutdown()
{
}

void CGraphics_SDL::Minimize()
{
}

void CGraphics_SDL::Maximize()
{
}

int CGraphics_SDL::WindowActive()
{
	return 1;
}

int CGraphics_SDL::WindowOpen()
{
	return 1;
}

void CGraphics_SDL::TakeScreenshot(const char *pFilename)
{
}

void CGraphics_SDL::Swap()
{
}


int CGraphics_SDL::GetVideoModes(CVideoMode *pModes, int MaxModes)
{
	pModes[0].m_Width = 640;
	pModes[0].m_Height = 480;
	pModes[0].m_Red = 8;
	pModes[0].m_Green = 8;
	pModes[0].m_Blue = 8;

	return 1;
}

// syncronization
void CGraphics_SDL::InsertSignal(semaphore *pSemaphore)
{
}

bool CGraphics_SDL::IsIdle()
{
	return true;
}

void CGraphics_SDL::WaitForIdle()
{
}

extern IEngineGraphics *CreateEngineGraphics() { return new CGraphics_SDL(); }
extern IEngineGraphics *CreateEngineGraphicsThreaded() { return new CGraphics_SDL(); }

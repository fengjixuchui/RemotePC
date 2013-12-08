#pragma once
//----------------------------------------------------------------------//
#define WIN32_LEAN_AND_MEAN 
#define VC_LEANMEAN         
//----------------------------------------------------------------------//
#include "Windows.h"
#include "stdio.h"
//----------------------------------------------------------------------//
#include "Screenshot.h"
#include "QuadTree.h"
//----------------------------------------------------------------------//

class CClientScreenshotManager : IClientScreenshotManager {
public:
	CClientScreenshotManager();
	~CClientScreenshotManager();
private:
	CRawBuffer UncompressedBuffer, OpenGLBuffer;
public:
	void Reset();
	void Decompress(BYTE *pCompressedBuffer, DWORD CompressedBufferSize, DecompressedScreenshotInfoStruct* pInfo);
};

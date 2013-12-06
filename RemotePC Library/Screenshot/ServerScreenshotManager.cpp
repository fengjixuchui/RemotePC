#include "ServerScreenshotManager.h"

CServerScreenshotManager::CServerScreenshotManager()
{
	Reset();
}

CServerScreenshotManager::~CServerScreenshotManager()
{
	Reset();
}

void CServerScreenshotManager::SwapBuffers()
{
	CScreenshot* pTmp = pBackBuffer;
	pBackBuffer  = pFrontBuffer;
	pFrontBuffer = pTmp;
}

void CServerScreenshotManager::Reset()
{
	pBackBuffer  = &Screenshot[0];
	pFrontBuffer = &Screenshot[1];

	pBackBuffer->Reset();
	pFrontBuffer->Reset();
}

CRawBuffer* CServerScreenshotManager::Take(BOOL bShowCursor)
{
	pBackBuffer->Take(bShowCursor);
	AdjustFrontBuffer();

    Pack();
	Compress();
	SwapBuffers();

	return &CompressedBuffer;
}

void CServerScreenshotManager::AdjustFrontBuffer()
{
	// Make sure an empty buffer is allocated if that's the first frame or if the screen size changed
	if(pFrontBuffer->GetBufferSize() != pBackBuffer->GetBufferSize())
		pFrontBuffer->CreateEmpty(pBackBuffer->GetWidth(), pBackBuffer->GetHeight(), pBackBuffer->GetBitsPerPixel());
}

int CServerScreenshotManager::Pack()
{
	// Define the max. depth of the recursion
	static const BYTE MaxDepth = 4; 

	BYTE *pFront = pFrontBuffer->GetBuffer();
	BYTE *pBack  = pBackBuffer->GetBuffer();
	
	WORD w   = pBackBuffer->GetWidth();
	WORD h   = pBackBuffer->GetHeight();
	BYTE bpp = pBackBuffer->GetBytesPerPixel();

	// Generate the full tree
	CQuadTree QuadTree;

	QuadTree.SetScreenshotBuffers(pFront, pBack);
	QuadTree.InitTree(w, h, bpp, MaxDepth);
	
	QuadTree.GenTree();
	QuadTree.TrimTree();
	
	// Calculate the size needed for the uncompressed buffer
	int UncompressedSize = QuadTree.CalcOutputBufferSize();

	// Extract the data in the uncompressed buffer
	UncompressedBuffer.Allocate(UncompressedSize);
	BYTE *pUnc = UncompressedBuffer.GetBuffer();

	QuadTree.SetUncompressedBuffer(pUnc);
	QuadTree.ExtractTreeData();

	// we're done with the tree
	QuadTree.KillTree();

	return UncompressedSize;
}

int CServerScreenshotManager::EstimateCompressedBufferSize(int UncompressedSize)
{
	return (int)(((float)UncompressedSize * 1.1f) + 12.0f);
}

int CServerScreenshotManager::Compress()
{
	// Store the uncompressed size and return 0 if empty
	int UncompressedSize = UncompressedBuffer.GetSize();
	if(UncompressedSize == 0)
		return 0;
	
	int CompressedHeaderSize = sizeof(CompressedScreenshotInfoStruct);
	int CompressedBufferSize = EstimateCompressedBufferSize(UncompressedSize) + CompressedHeaderSize;

	CompressedBuffer.Allocate(CompressedBufferSize);

	CompressedScreenshotInfoStruct CompressionHeader;
	CompressionHeader.Width  = pBackBuffer->GetWidth();
	CompressionHeader.Height = pBackBuffer->GetHeight();
	CompressionHeader.BitsPerPixel = pBackBuffer->GetBitsPerPixel();
	CompressionHeader.UncompressedSize = UncompressedSize;

	memcpy(CompressedBuffer.GetBuffer(), &CompressionHeader, CompressedHeaderSize);

	BYTE *pIn  = UncompressedBuffer.GetBuffer();
	BYTE *pOut = CompressedBuffer.GetBuffer(CompressedHeaderSize);

	int CompressedDataSize = FreeImage_ZLibCompress(pOut, CompressedBufferSize, pIn, UncompressedSize);

	CompressedBufferSize = CompressedHeaderSize + CompressedDataSize;
	CompressedBuffer.Resize(CompressedBufferSize);

	return CompressedDataSize;
}

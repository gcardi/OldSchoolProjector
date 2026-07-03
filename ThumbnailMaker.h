//---------------------------------------------------------------------------

#ifndef ThumbnailMakerH
#define ThumbnailMakerH

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Graphics.hpp>

#include <memory>

//---------------------------------------------------------------------------

// Decode the image at FileName and return an FMX bitmap scaled to fit within
// MaxW x MaxH, preserving aspect ratio and never upscaling. Uses WIC, so it
// handles every format WIC supports (jpg, png, ...). Throws on failure.
std::unique_ptr<Fmx::Graphics::TBitmap>
    MakeThumbnail( System::UnicodeString FileName, int MaxW, int MaxH );

//---------------------------------------------------------------------------
#endif

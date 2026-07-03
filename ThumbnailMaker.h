//---------------------------------------------------------------------------

#ifndef ThumbnailMakerH
#define ThumbnailMakerH

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Graphics.hpp>

#include <memory>

//---------------------------------------------------------------------------

// Decode FileName with WIC, apply its EXIF orientation (auto-rotate), and
// return an FMX bitmap. If MaxW > 0 and MaxH > 0 the result is scaled to fit
// within that box (aspect preserved, never upscaled) - used for thumbnails;
// pass 0 (or negative) for both to get the full-resolution image - used for the
// projected picture. Throws on failure; returns nullptr for an empty image.
std::unique_ptr<Fmx::Graphics::TBitmap>
    LoadImageOriented( System::UnicodeString FileName, int MaxW, int MaxH );

//---------------------------------------------------------------------------
#endif

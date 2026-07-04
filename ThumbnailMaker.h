//---------------------------------------------------------------------------

#ifndef ThumbnailMakerH
#define ThumbnailMakerH

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Graphics.hpp>

#include <memory>
#include <set>
#include <string>

//---------------------------------------------------------------------------

// Return the set of lower-cased file extensions (each with its leading dot,
// e.g. L".jpg") that the WIC codecs installed on this machine can decode.
// Because the list is queried from WIC at runtime, installing an extra codec
// - for instance the Microsoft/Canon/Nikon RAW camera codec packs - makes the
// matching files (.cr2, .cr3, .nef, .arw, ...) show up here automatically,
// with no change to the program. Returns an empty set if WIC cannot be queried.
std::set<std::wstring> GetWICSupportedExtensions();

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

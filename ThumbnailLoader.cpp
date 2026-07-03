//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <objbase.h>

#include <algorithm>

#include "ThumbnailLoader.h"
#include "ThumbnailMaker.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

//---------------------------------------------------------------------------
TThumbnailLoader::TThumbnailLoader( int TargetSize )
    : targetSize_( TargetSize )
{
    // Start the worker only once every member above is fully constructed.
    thread_ = std::thread( &TThumbnailLoader::Run, this );
}
//---------------------------------------------------------------------------

TThumbnailLoader::~TThumbnailLoader()
{
    {
        std::unique_lock<std::mutex> Lock( mutex_ );
        stop_ = true;
    }
    cv_.notify_one();
    if ( thread_.joinable() ) {
        thread_.join();
    }
}
//---------------------------------------------------------------------------

void TThumbnailLoader::SetEntries( std::vector<System::UnicodeString> Paths )
{
    {
        std::unique_lock<std::mutex> Lock( mutex_ );
        paths_ = std::move( Paths );
        cache_.clear();
        ++generation_;
        wantFirst_ = 0;
        wantLast_ = -1;
        dirty_ = true;
        ready_ = true;   // cache changed: let the UI repaint (placeholders)
    }
    cv_.notify_one();
}
//---------------------------------------------------------------------------

void TThumbnailLoader::EnsureRange( int First, int Last )
{
    {
        std::unique_lock<std::mutex> Lock( mutex_ );
        int const N = static_cast<int>( paths_.size() );
        wantFirst_ = std::max( 0, First - PreloadMargin );
        wantLast_ = std::min( N - 1, Last + PreloadMargin );
        dirty_ = true;
    }
    cv_.notify_one();
}
//---------------------------------------------------------------------------

Fmx::Graphics::TBitmap* TThumbnailLoader::Get( int Index )
{
    std::unique_lock<std::mutex> Lock( mutex_ );
    auto It = cache_.find( Index );
    return It != cache_.end() ? It->second.get() : nullptr;
}
//---------------------------------------------------------------------------

bool TThumbnailLoader::ConsumeReady()
{
    std::unique_lock<std::mutex> Lock( mutex_ );
    bool const Was = ready_;
    ready_ = false;
    return Was;
}
//---------------------------------------------------------------------------

int TThumbnailLoader::NextMissingLocked()
{
    int const Last = std::min( wantLast_, static_cast<int>( paths_.size() ) - 1 );
    for ( int I = std::max( 0, wantFirst_ ); I <= Last; ++I ) {
        if ( cache_.find( I ) == cache_.end() ) {
            return I;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------

void TThumbnailLoader::Run()
{
    // WIC needs COM on this thread.
    CoInitializeEx( nullptr, COINIT_MULTITHREADED );

    for ( ;; ) {
        int Index = -1;
        System::UnicodeString Path;
        unsigned Gen = 0;

        {
            std::unique_lock<std::mutex> Lock( mutex_ );
            cv_.wait(
                Lock,
                [this] { return stop_ || dirty_ || NextMissingLocked() >= 0; }
            );
            if ( stop_ ) {
                break;
            }
            dirty_ = false;
            Index = NextMissingLocked();
            if ( Index >= 0 ) {
                Path = paths_[Index];
                Gen = generation_;
            }
        }

        if ( Index < 0 ) {
            continue;
        }

        // Heavy work outside the lock.
        std::unique_ptr<Fmx::Graphics::TBitmap> Bmp;
        try {
            Bmp = MakeThumbnail( Path, targetSize_, targetSize_ );
        }
        catch ( ... ) {
            Bmp.reset();   // cache the failure as null so we do not retry it
        }

        {
            std::unique_lock<std::mutex> Lock( mutex_ );
            // Discard the result if the list was replaced while we worked.
            if ( Gen == generation_ && Index < static_cast<int>( paths_.size() ) ) {
                cache_[Index] = std::move( Bmp );
                ready_ = true;
            }
        }
    }

    CoUninitialize();
}
//---------------------------------------------------------------------------

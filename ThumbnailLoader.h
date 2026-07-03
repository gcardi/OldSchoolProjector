//---------------------------------------------------------------------------

#ifndef ThumbnailLoaderH
#define ThumbnailLoaderH

//---------------------------------------------------------------------------

#include <FMX.Graphics.hpp>

#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

//---------------------------------------------------------------------------

// Background thumbnail producer. It lives on the host side (the frame stays
// cache-agnostic) and owns the thumbnail cache. The UI thread pushes the
// wanted (visible) index range; a single std::thread worker decodes/scales the
// missing thumbnails off the UI thread and caches them. It only ever touches
// the cache (under the mutex) and a "ready" flag - never FMX UI objects - so
// the host just polls ConsumeReady() from a timer and repaints when needed.
class TThumbnailLoader
{
public:
    explicit TThumbnailLoader( int TargetSize );
    ~TThumbnailLoader();

    TThumbnailLoader( TThumbnailLoader const& ) = delete;
    TThumbnailLoader& operator=( TThumbnailLoader const& ) = delete;

    // --- all called from the UI thread ---

    // Replace the list of source paths (index -> path) and drop the cache.
    void SetEntries( std::vector<System::UnicodeString> Paths );
    // Ask for the given visible range to be loaded (a margin is preloaded too).
    void EnsureRange( int First, int Last );
    // Cached thumbnail for Index, or nullptr if not ready yet.
    Fmx::Graphics::TBitmap* Get( int Index );
    // True (once) if new thumbnails have arrived since the previous call.
    bool ConsumeReady();

private:
    static constexpr int PreloadMargin = 8;   // extra items loaded around view

    int const targetSize_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<System::UnicodeString> paths_;
    std::map<int, std::unique_ptr<Fmx::Graphics::TBitmap>> cache_;
    int wantFirst_ { 0 };
    int wantLast_ { -1 };
    unsigned generation_ { 0 };   // bumped on SetEntries to drop stale results
    bool dirty_ { false };
    bool ready_ { false };
    bool stop_ { false };

    std::thread thread_;   // constructed last, in the body, once all is ready

    int NextMissingLocked();      // caller holds mutex_; -1 if nothing to do
    void Run();
};

//---------------------------------------------------------------------------
#endif

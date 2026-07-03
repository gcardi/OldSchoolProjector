//---------------------------------------------------------------------------

#ifndef ThumbnailStripH
#define ThumbnailStripH

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <System.UITypes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Types.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Objects.hpp>
#include <FMX.Graphics.hpp>
#include <FMX.Controls.Presentation.hpp>

//---------------------------------------------------------------------------

// The frame knows nothing about the filesystem or the thumbnail cache: it only
// draws and scrolls. It asks the host for the data it needs through these
// closures, and reports user interaction back the same way.

// Host returns the cached bitmap for Index, or leaves Bmp = nullptr if it is
// not (yet) available (the frame then draws a placeholder). The frame does not
// own the bitmap.
typedef void __fastcall ( __closure *TThumbRequestEvent )
    ( TObject* Sender, int Index, TBitmap*& Bmp );

// User clicked the thumbnail at Index.
typedef void __fastcall ( __closure *TThumbPickEvent )
    ( TObject* Sender, int Index );

// The visible window changed (scroll/resize/count): First..Last inclusive.
// The host uses this to drive lazy loading / preloading (later steps).
typedef void __fastcall ( __closure *TThumbRangeEvent )
    ( TObject* Sender, int First, int Last );

//---------------------------------------------------------------------------

class TThumbnailStrip : public TFrame
{
__published:    // IDE-managed Components
    TSmallScrollBar *scrollThumbs;
    TPaintBox *paintThumbs;
    void __fastcall paintThumbsPaint( TObject *Sender, TCanvas *Canvas,
                                      const TRectF &ARect );
    void __fastcall scrollThumbsChange( TObject *Sender );
    void __fastcall paintThumbsMouseDown( TObject *Sender, TMouseButton Button,
                                          TShiftState Shift, float X, float Y );
    void __fastcall paintThumbsResized( TObject *Sender );

private:    // User declarations
    int count_ {};
    int selectedIndex_ { -1 };
    float thumbAspectRatio_ { 16.0f / 9.0f };

    TThumbRequestEvent onRequestThumbnail_ {};
    TThumbPickEvent    onPick_ {};
    TThumbRangeEvent   onVisibleRangeChanged_ {};

    static constexpr float Margin = 4.0f;    // gap between strip edge and thumbs
    static constexpr float Spacing = 4.0f;   // gap between two thumbs

    void SetCount( int Val );
    void SetSelectedIndex( int Val );
    void SetThumbAspectRatio( float Val );

    float ThumbHeight() const;
    float ThumbWidth() const;
    int VisibleCount() const;
    int FirstVisibleIndex() const;
    int IndexAtX( float X ) const;
    void UpdateScrollRange();
    void NotifyVisibleRange();
    void EnsureVisible( int Index );

public:    // User declarations
    __fastcall TThumbnailStrip( TComponent* Owner );

    // Number of thumbnails to represent (no filesystem knowledge here).
    __property int Count = { read = count_, write = SetCount };
    // Highlighted thumbnail = the picture currently projected; -1 = none.
    __property int SelectedIndex = {
        read = selectedIndex_, write = SetSelectedIndex
    };
    // Thumbnail shape (width/height); the host passes the canvas aspect ratio.
    __property float ThumbAspectRatio = {
        read = thumbAspectRatio_, write = SetThumbAspectRatio
    };

    __property TThumbRequestEvent OnRequestThumbnail = {
        read = onRequestThumbnail_, write = onRequestThumbnail_
    };
    __property TThumbPickEvent OnPick = {
        read = onPick_, write = onPick_
    };
    __property TThumbRangeEvent OnVisibleRangeChanged = {
        read = onVisibleRangeChanged_, write = onVisibleRangeChanged_
    };

    // The host calls this once a thumbnail becomes available (async, later
    // steps) so the frame can repaint it if it is currently visible.
    void ThumbnailReady( int Index );
};

//---------------------------------------------------------------------------
#endif

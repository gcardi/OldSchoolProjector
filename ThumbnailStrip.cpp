//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <System.UIConsts.hpp>

#include <algorithm>
#include <cmath>

#include "ThumbnailStrip.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"

using std::floor;
using std::max;
using std::min;

//---------------------------------------------------------------------------
__fastcall TThumbnailStrip::TThumbnailStrip( TComponent* Owner )
    : TFrame( Owner )
{
}
//---------------------------------------------------------------------------

float TThumbnailStrip::ThumbHeight() const
{
    return max( 0.0f, paintThumbs->Height - 2 * Margin );
}
//---------------------------------------------------------------------------

float TThumbnailStrip::ThumbWidth() const
{
    return ThumbHeight() * thumbAspectRatio_;
}
//---------------------------------------------------------------------------

int TThumbnailStrip::VisibleCount() const
{
    float const Slot = ThumbWidth() + Spacing;
    if ( Slot <= 0 ) {
        return 0;
    }
    float const Avail = paintThumbs->Width - 2 * Margin + Spacing;
    return max( 0, static_cast<int>( floor( Avail / Slot ) ) );
}
//---------------------------------------------------------------------------

int TThumbnailStrip::FirstVisibleIndex() const
{
    return static_cast<int>( scrollThumbs->Value );
}
//---------------------------------------------------------------------------

void TThumbnailStrip::UpdateScrollRange()
{
    int const Vis = VisibleCount();
    scrollThumbs->Min = 0;
    scrollThumbs->Max = count_;
    scrollThumbs->ViewportSize = Vis;
    scrollThumbs->SmallChange = 1;
    scrollThumbs->Enabled = count_ > Vis;
    if ( FirstVisibleIndex() > count_ - Vis ) {
        scrollThumbs->Value = max( 0, count_ - Vis );
    }
}
//---------------------------------------------------------------------------

void TThumbnailStrip::EnsureVisible( int Index )
{
    if ( Index < 0 ) {
        return;
    }
    int const Vis = VisibleCount();
    if ( Vis <= 0 ) {
        return;
    }
    // Keep the selected thumbnail centred, so the previous and next slides stay
    // visible on either side. The scrollbar naturally clamps near the two ends
    // (the selection then sits off-centre because we cannot scroll past them).
    int const MaxFirst = max( 0, count_ - Vis );
    scrollThumbs->Value = max( 0, min( Index - Vis / 2, MaxFirst ) );
}
//---------------------------------------------------------------------------

void TThumbnailStrip::NotifyVisibleRange()
{
    if ( onVisibleRangeChanged_ && count_ > 0 ) {
        int const First = FirstVisibleIndex();
        int const Last = min( count_ - 1, First + VisibleCount() - 1 );
        onVisibleRangeChanged_( this, First, Last );
    }
}
//---------------------------------------------------------------------------

int TThumbnailStrip::IndexAtX( float X ) const
{
    float const Slot = ThumbWidth() + Spacing;
    if ( Slot <= 0 ) {
        return -1;
    }
    float const Rel = X - Margin;
    if ( Rel < 0 ) {
        return -1;
    }
    int const Col = static_cast<int>( floor( Rel / Slot ) );
    if ( Col >= VisibleCount() ) {
        return -1;
    }
    if ( Rel - Col * Slot > ThumbWidth() ) {   // clicked in the gap
        return -1;
    }
    int const Idx = FirstVisibleIndex() + Col;
    return ( Idx >= 0 && Idx < count_ ) ? Idx : -1;
}
//---------------------------------------------------------------------------

void TThumbnailStrip::SetCount( int Val )
{
    count_ = max( 0, Val );
    if ( selectedIndex_ >= count_ ) {
        selectedIndex_ = -1;
    }
    UpdateScrollRange();
    paintThumbs->Repaint();
    NotifyVisibleRange();
}
//---------------------------------------------------------------------------

void TThumbnailStrip::SetSelectedIndex( int Val )
{
    int const New = ( Val >= 0 && Val < count_ ) ? Val : -1;
    if ( New != selectedIndex_ ) {
        selectedIndex_ = New;
        // The selection only changes when the projected picture changes (e.g.
        // Next/Prev), so scroll it into view. Plain scrolling does not touch
        // the selection, so it never triggers this.
        EnsureVisible( selectedIndex_ );
        paintThumbs->Repaint();
    }
}
//---------------------------------------------------------------------------

void TThumbnailStrip::SetThumbAspectRatio( float Val )
{
    if ( Val > 0 && Val != thumbAspectRatio_ ) {
        thumbAspectRatio_ = Val;
        UpdateScrollRange();
        paintThumbs->Repaint();
        NotifyVisibleRange();
    }
}
//---------------------------------------------------------------------------

void TThumbnailStrip::RefreshThumbnails()
{
    paintThumbs->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TThumbnailStrip::paintThumbsResized( TObject* /*Sender*/ )
{
    UpdateScrollRange();
    paintThumbs->Repaint();
    NotifyVisibleRange();
}
//---------------------------------------------------------------------------

void __fastcall TThumbnailStrip::scrollThumbsChange( TObject* /*Sender*/ )
{
    paintThumbs->Repaint();
    NotifyVisibleRange();
}
//---------------------------------------------------------------------------

void __fastcall TThumbnailStrip::paintThumbsMouseDown(
    TObject* /*Sender*/, TMouseButton Button, TShiftState /*Shift*/,
    float X, float /*Y*/ )
{
    if ( Button != TMouseButton::mbLeft ) {
        return;
    }
    int const Idx = IndexAtX( X );
    if ( Idx >= 0 && onPick_ ) {
        onPick_( this, Idx );
    }
}
//---------------------------------------------------------------------------

void __fastcall TThumbnailStrip::paintThumbsPaint(
    TObject* /*Sender*/, TCanvas* Canvas, const TRectF& /*ARect*/ )
{
    float const Th = ThumbHeight();
    float const Tw = ThumbWidth();
    if ( Th <= 0 || Tw <= 0 ) {
        return;
    }

    int const First = FirstVisibleIndex();
    int const Vis = VisibleCount();
    float X = Margin;
    float const Y = Margin;

    for ( int i = First; i < First + Vis && i < count_; ++i ) {
        TRectF const R( X, Y, X + Tw, Y + Th );

        // Ask the host for a cached thumbnail; nullptr => draw a placeholder.
        TBitmap* Bmp = nullptr;
        if ( onRequestThumbnail_ ) {
            onRequestThumbnail_( this, i, Bmp );
        }

        if ( Bmp ) {
            // Letterbox the thumbnail inside the slot, preserving its aspect
            // ratio (contain): fill the slot black, then centre the picture.
            Canvas->Fill->Kind = TBrushKind::Solid;
            Canvas->Fill->Color = claBlack;
            Canvas->FillRect( R, 0, 0, AllCorners, 1.0f );

            float const BmpAR = static_cast<float>( Bmp->Width ) / Bmp->Height;
            float const SlotAR = Tw / Th;
            TRectF Dest = R;
            if ( BmpAR > SlotAR ) {
                float const NewH = Tw / BmpAR;
                Dest.Top = R.Top + ( Th - NewH ) / 2;
                Dest.Bottom = Dest.Top + NewH;
            }
            else {
                float const NewW = Th * BmpAR;
                Dest.Left = R.Left + ( Tw - NewW ) / 2;
                Dest.Right = Dest.Left + NewW;
            }
            Canvas->DrawBitmap(
                Bmp, TRectF( 0, 0, Bmp->Width, Bmp->Height ), Dest, 1.0f, true
            );
        }
        else {
            Canvas->Fill->Kind = TBrushKind::Solid;
            Canvas->Fill->Color = claGray;
            Canvas->FillRect( R, 0, 0, AllCorners, 1.0f );
            Canvas->Fill->Color = claWhitesmoke;
            Canvas->FillText(
                R, IntToStr( i ), false, 1.0f, TFillTextFlags(),
                TTextAlign::Center, TTextAlign::Center
            );
        }

        if ( i == selectedIndex_ ) {
            Canvas->Stroke->Kind = TBrushKind::Solid;
            Canvas->Stroke->Color = claYellow;
            Canvas->Stroke->Thickness = 3.0f;
            Canvas->DrawRect( R, 0, 0, AllCorners, 1.0f );
        }

        X += Tw + Spacing;
    }
}
//---------------------------------------------------------------------------

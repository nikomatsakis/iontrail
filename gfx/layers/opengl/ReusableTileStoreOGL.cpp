/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ReusableTileStoreOGL.h"

namespace mozilla {
namespace layers {

ReusableTileStoreOGL::~ReusableTileStoreOGL()
{
  if (mTiles.Length() == 0)
    return;

  mContext->MakeCurrent();
  for (uint32_t i = 0; i < mTiles.Length(); i++)
    mContext->fDeleteTextures(1, &mTiles[i]->mTexture.mTextureHandle);
  mTiles.Clear();
}

void
ReusableTileStoreOGL::InvalidateTiles(TiledThebesLayerOGL* aLayer,
                                      const nsIntRegion& aValidRegion,
                                      const gfxSize& aResolution)
{
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Invalidating reused tiles\n");
#endif

  // Find out the area of the nearest display-port to invalidate retained
  // tiles.
  gfxRect renderBounds;
  for (ContainerLayer* parent = aLayer->GetParent(); parent; parent = parent->GetParent()) {
      const FrameMetrics& metrics = parent->GetFrameMetrics();
      if (!metrics.mDisplayPort.IsEmpty()) {
          // We use the bounds to cut down on complication/computation time.
          // This will be incorrect when the transform involves rotation, but
          // it'd be quite hard to retain invalid tiles correctly in this
          // situation anyway.
          renderBounds = parent->GetEffectiveTransform().TransformBounds(
              gfxRect(metrics.mDisplayPort.x, metrics.mDisplayPort.y,
                      metrics.mDisplayPort.width, metrics.mDisplayPort.height));
          break;
      }
  }

  // If no display port was found, use the widget size from the layer manager.
  if (renderBounds.IsEmpty()) {
      LayerManagerOGL* manager = static_cast<LayerManagerOGL*>(aLayer->Manager());
      const nsIntSize& widgetSize = manager->GetWidgetSize();
      renderBounds.width = widgetSize.width;
      renderBounds.height = widgetSize.height;
  }

  // Iterate over existing harvested tiles and release any that are contained
  // within the new valid region, the display-port or the widget area. The
  // assumption is that anything within this area should be valid, so there's
  // no need to keep invalid tiles there.
  mContext->MakeCurrent();
  for (uint32_t i = 0; i < mTiles.Length();) {
    ReusableTiledTextureOGL* tile = mTiles[i];
    bool release = false;

    nsIntRect tileRect = tile->mTileRegion.GetBounds();
    if (tile->mResolution != aResolution) {
      nsIntRegion transformedTileRegion(tile->mTileRegion);
      transformedTileRegion.ScaleRoundOut(tile->mResolution.width / aResolution.width,
                                          tile->mResolution.height / aResolution.height);
      tileRect = transformedTileRegion.GetBounds();
    }

    // Check if the tile region is contained within the new valid region.
    if (aValidRegion.Contains(tileRect)) {
      release = true;
    }

    if (release) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
      nsIntRect tileBounds = tile->mTileRegion.GetBounds();
      printf_stderr("Releasing obsolete reused tile at %d,%d, x%f\n",
                    tileBounds.x, tileBounds.y, tile->mResolution.width);
#endif
      mContext->fDeleteTextures(1, &tile->mTexture.mTextureHandle);
      mTiles.RemoveElementAt(i);
      continue;
    }

    i++;
  }
}

void
ReusableTileStoreOGL::HarvestTiles(TiledThebesLayerOGL* aLayer,
                                   TiledLayerBufferOGL* aVideoMemoryTiledBuffer,
                                   const nsIntRegion& aOldValidRegion,
                                   const nsIntRegion& aNewValidRegion,
                                   const gfxSize& aOldResolution,
                                   const gfxSize& aNewResolution)
{
  gfxSize scaleFactor = gfxSize(aNewResolution.width / aOldResolution.width,
                                aNewResolution.height / aOldResolution.height);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Seeing if there are any tiles we can reuse\n");
#endif

  // Iterate over the tiles and decide which ones we're going to harvest.
  // We harvest any tile that is entirely outside of the new valid region, or
  // any tile that is partially outside of the valid region and whose
  // resolution has changed.
  // XXX Tile iteration needs to be abstracted, or have some utility functions
  //     to make it simpler.
  uint16_t tileSize = aVideoMemoryTiledBuffer->GetTileLength();
  nsIntRect validBounds = aOldValidRegion.GetBounds();
  for (int x = validBounds.x; x < validBounds.XMost();) {
    int w = tileSize - aVideoMemoryTiledBuffer->GetTileStart(x);
    if (x + w > validBounds.x + validBounds.width)
      w = validBounds.x + validBounds.width - x;

    // A tile will consume 256^2 of memory, don't retain small tile trims.
    // This works around the display port sometimes creating a small 1 pixel wide
    // tile because of rounding error.
    if (w < 16) {
      x += w;
      continue;
    }

    for (int y = validBounds.y; y < validBounds.YMost();) {
      int h = tileSize - aVideoMemoryTiledBuffer->GetTileStart(y);
      if (y + h > validBounds.y + validBounds.height)
        h = validBounds.y + validBounds.height - y;

      if (h < 16) {
        y += h;
        continue;
      }

      // If the new valid region doesn't contain this tile region,
      // harvest the tile.
      nsIntRegion tileRegion;
      tileRegion.And(aOldValidRegion, nsIntRect(x, y, w, h));

      nsIntRegion intersectingRegion;
      bool retainTile = false;
      if (aNewResolution != aOldResolution) {
        // Reconcile resolution changes.
        // If the resolution changes, we know the backing layer will have been
        // invalidated, so retain tiles that are partially encompassed by the
        // new valid area, instead of just tiles that don't intersect at all.
        nsIntRegion transformedTileRegion(tileRegion);
        transformedTileRegion.ScaleRoundOut(scaleFactor.width, scaleFactor.height);
        if (!aNewValidRegion.Contains(transformedTileRegion))
          retainTile = true;
      } else if (intersectingRegion.And(tileRegion, aNewValidRegion).IsEmpty()) {
        retainTile = true;
      }

      if (retainTile) {
        TiledTexture removedTile;
        if (aVideoMemoryTiledBuffer->RemoveTile(nsIntPoint(x, y), removedTile)) {
          ReusableTiledTextureOGL* reusedTile =
            new ReusableTiledTextureOGL(removedTile, nsIntPoint(x, y), tileRegion,
                                        tileSize, aOldResolution);
          mTiles.AppendElement(reusedTile);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
          bool replacedATile = false;
#endif
          // Remove any tile that is superseded by this new tile.
          // (same resolution, same area)
          for (int i = 0; i < mTiles.Length() - 1; i++) {
            // XXX Perhaps we should check the region instead of the origin
            //     so a partial tile doesn't replace a full older tile?
            if (aVideoMemoryTiledBuffer->RoundDownToTileEdge(mTiles[i]->mTileOrigin.x) == aVideoMemoryTiledBuffer->RoundDownToTileEdge(x) &&
                aVideoMemoryTiledBuffer->RoundDownToTileEdge(mTiles[i]->mTileOrigin.y) == aVideoMemoryTiledBuffer->RoundDownToTileEdge(y) &&
                mTiles[i]->mResolution == aOldResolution) {
              mContext->fDeleteTextures(1, &mTiles[i]->mTexture.mTextureHandle);
              mTiles.RemoveElementAt(i);
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
              replacedATile = true;
#endif
              // There should only be one similar tile
              break;
            }
          }
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
          if (replacedATile) {
            printf_stderr("Replaced tile at %d,%d, x%f for reuse\n", x, y, aOldResolution.width);
          } else {
            printf_stderr("New tile at %d,%d, x%f for reuse\n", x, y, aOldResolution.width);
          }
#endif
        }
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
        else
          printf_stderr("Failed to retain tile for reuse\n");
#endif
      }

      y += h;
    }

    x += w;
  }

  // Make sure we don't hold onto tiles that may cause visible rendering glitches
  InvalidateTiles(aLayer, aNewValidRegion, aNewResolution);

  // Now prune our reused tile store of its oldest tiles if it gets too large.
  while (mTiles.Length() > aVideoMemoryTiledBuffer->GetTileCount() * mSizeLimit) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    nsIntRect tileBounds = mTiles[0]->mTileRegion.GetBounds();
    printf_stderr("Releasing old reused tile at %d,%d, x%f\n",
                  tileBounds.x, tileBounds.y, mTiles[0]->mResolution.width);
#endif
    mContext->fDeleteTextures(1, &mTiles[0]->mTexture.mTextureHandle);
    mTiles.RemoveElementAt(0);
  }

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Retained tile limit: %f\n", aVideoMemoryTiledBuffer->GetTileCount() * mSizeLimit);
  printf_stderr("Retained %d tiles\n", mTiles.Length());
#endif
}

void
ReusableTileStoreOGL::DrawTiles(TiledThebesLayerOGL* aLayer,
                                const nsIntRegion& aValidRegion,
                                const gfxSize& aResolution,
                                const gfx3DMatrix& aTransform,
                                const nsIntPoint& aRenderOffset,
                                Layer* aMaskLayer)
{
  // Walk up the tree, looking for a display-port - if we find one, we know
  // that this layer represents a content node and we can use its first
  // scrollable child, in conjunction with its content area and viewport offset
  // to establish the screen coordinates to which the content area will be
  // rendered.
  gfxRect contentBounds, displayPort;
  ContainerLayer* scrollableLayer = nullptr;
  for (ContainerLayer* parent = aLayer->GetParent(); parent; parent = parent->GetParent()) {
      const FrameMetrics& parentMetrics = parent->GetFrameMetrics();
      if (parentMetrics.IsScrollable())
        scrollableLayer = parent;
      if (!parentMetrics.mDisplayPort.IsEmpty() && scrollableLayer) {
          displayPort = parent->GetEffectiveTransform().
            TransformBounds(gfxRect(
              parentMetrics.mDisplayPort.x, parentMetrics.mDisplayPort.y,
              parentMetrics.mDisplayPort.width, parentMetrics.mDisplayPort.height));
          const FrameMetrics& metrics = scrollableLayer->GetFrameMetrics();
          const nsIntSize& contentSize = metrics.mContentRect.Size();
          gfx::Point scrollOffset =
            gfx::Point(metrics.mScrollOffset.x * metrics.LayersPixelsPerCSSPixel().width,
                       metrics.mScrollOffset.y * metrics.LayersPixelsPerCSSPixel().height);
          const nsIntPoint& contentOrigin = metrics.mContentRect.TopLeft() -
            nsIntPoint(NS_lround(scrollOffset.x), NS_lround(scrollOffset.y));
          gfxRect contentRect = gfxRect(contentOrigin.x, contentOrigin.y,
                                        contentSize.width, contentSize.height);
          contentBounds = scrollableLayer->GetEffectiveTransform().TransformBounds(contentRect);
          break;
      }
  }

  // Render old tiles to fill in gaps we haven't had the time to render yet.
  for (uint32_t i = 0; i < mTiles.Length(); i++) {
    ReusableTiledTextureOGL* tile = mTiles[i];

    // Work out the scaling factor in case of resolution differences.
    gfxSize scaleFactor = gfxSize(aResolution.width / tile->mResolution.width,
                                  aResolution.height / tile->mResolution.height);

    // Reconcile the resolution difference by adjusting the transform.
    gfx3DMatrix transform = aTransform;
    if (aResolution != tile->mResolution)
      transform.Scale(scaleFactor.width, scaleFactor.height, 1);

    // Subtract the layer's valid region from the tile region.
    // This region will be drawn by the layer itself.
    nsIntRegion transformedValidRegion(aValidRegion);
    if (aResolution != tile->mResolution)
      transformedValidRegion.ScaleRoundOut(1.0f/scaleFactor.width,
                                           1.0f/scaleFactor.height);
    nsIntRegion tileRegion;
    tileRegion.Sub(tile->mTileRegion, transformedValidRegion);

    // Subtract the display-port from the tile region.
    if (!displayPort.IsEmpty()) {
      gfxRect transformedRenderBounds = transform.Inverse().TransformBounds(displayPort);
      tileRegion.Sub(tileRegion, nsIntRect(transformedRenderBounds.x,
                                           transformedRenderBounds.y,
                                           transformedRenderBounds.width,
                                           transformedRenderBounds.height));
    }

    // Intersect the tile region with the content area.
    if (!contentBounds.IsEmpty()) {
      gfxRect transformedRenderBounds = transform.Inverse().TransformBounds(contentBounds);
      tileRegion.And(tileRegion, nsIntRect(transformedRenderBounds.x,
                                           transformedRenderBounds.y,
                                           transformedRenderBounds.width,
                                           transformedRenderBounds.height));
    }

    // If the tile region is empty, skip drawing.
    if (tileRegion.IsEmpty())
      continue;

    // XXX If we have multiple tiles covering the same area, we will
    //     end up with rendering artifacts if the aLayer isn't opaque.
    int32_t tileStartX;
    int32_t tileStartY;
    if (tile->mTileOrigin.x >= 0) {
      tileStartX = tile->mTileOrigin.x % tile->mTileSize;
    } else {
      tileStartX = (tile->mTileSize - (-tile->mTileOrigin.x % tile->mTileSize)) % tile->mTileSize;
    }
    if (tile->mTileOrigin.y >= 0) {
      tileStartY = tile->mTileOrigin.y % tile->mTileSize;
    } else {
      tileStartY = (tile->mTileSize - (-tile->mTileOrigin.y % tile->mTileSize)) % tile->mTileSize;
    }
    nsIntPoint tileOffset(tile->mTileOrigin.x - tileStartX, tile->mTileOrigin.y - tileStartY);
    nsIntSize textureSize(tile->mTileSize, tile->mTileSize);
    aLayer->RenderTile(tile->mTexture, transform, aRenderOffset, tileRegion, tileOffset, textureSize, aMaskLayer);
  }
}

} // mozilla
} // layers

#include "Device.h"

#include "Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLink.h"

#include <cstdio>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace Device::KeyPad::MPE
{
namespace
{
struct TouchPoint {
  uint8_t trackingId = 0xFF;
  uint16_t xQ8 = 0U;
  uint16_t yQ8 = 0U;
  uint16_t pressure = 0U;
  uint16_t rawXQ8 = 0U;
  uint16_t rawYQ8 = 0U;
  uint16_t rawPressure = 0U;
  bool active = false;
  bool matched = false;
  bool justStarted = false;
  bool justEnded = false;
  bool dirty = false;
  uint8_t missingFrames = 0U;
  uint32_t lastSeenMs = 0U;
};

struct TouchBlob {
  float centerX = 0.0f;
  float centerY = 0.0f;
  float pressure = 0.0f;
  uint8_t minX = 0U;
  uint8_t minY = 0U;
  uint8_t maxX = 0U;
  uint8_t maxY = 0U;
  bool valid = false;
};

constexpr char kTag[] = "Mystrix2-MPE";
constexpr uint8_t kGridSize = MPECoprocessorLink::MPE_DATA_SIZE;
constexpr uint8_t kMaxTouchPoints = MULTIPRESS;
constexpr uint8_t kInvalidTrackingId = 0xFF;
constexpr uint8_t kMaxMissingFrames = 2U;
constexpr uint16_t kTouchBlobThreshold = 28000U;
constexpr uint16_t kTouchOnThreshold = 42000U;
constexpr uint16_t kTouchHysteresis = 8000U;
constexpr uint64_t kTouchTimeoutUs = 100000U;
constexpr int8_t kTouchMergeRadius = 2;
constexpr uint16_t kPositionAlphaX1000 = 300U;
constexpr uint16_t kPressureAlphaX1000 = 350U;
constexpr uint16_t kPadSpanQ8 = MPECoprocessorLink::SECTOR_GRID_SIZE * 256U;
constexpr uint16_t kSamePadAxisLimitQ8 = 320U;
constexpr uint16_t kNeighborPrimaryAxisLimitQ8 = 896U;
constexpr uint16_t kNeighborSecondaryAxisLimitQ8 = 224U;
constexpr uint16_t kDuplicatePrimaryAxisLimitQ8 = 192U;
constexpr uint16_t kDuplicateSecondaryAxisLimitQ8 = 96U;
constexpr uint16_t kBoundaryLocalThresholdQ8 = 192U;
constexpr uint8_t kBlobMergeGap = 1U;
constexpr uint8_t kAxisMergePrimaryGap = 2U;
constexpr uint8_t kAxisMergeSecondaryGap = 1U;
constexpr uint8_t kAxisMergeCombinedSpan = (MPECoprocessorLink::SECTOR_GRID_SIZE * 2U) + 1U;
MPECoprocessorLink coprocessorLink;
TouchPoint* touchPoints = nullptr;
uint64_t lastScanUs = 0U;
uint8_t nextTrackingId = 0U;
float scanRateHz = 0.0f;

IRAM_ATTR uint16_t QuantizeCoord(float value) {
  if (value <= 0.0f)
  {
    return 0U;
  }

  const float scaled = value * 256.0f;
  if (scaled >= 65535.0f)
  {
    return UINT16_MAX;
  }

  return (uint16_t)(scaled + 0.5f);
}

IRAM_ATTR float DequantizeCoord(uint16_t valueQ8) {
  return (float)valueQ8 / 256.0f;
}

IRAM_ATTR uint16_t AbsDiffU16(uint16_t a, uint16_t b) {
  return (a >= b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

IRAM_ATTR uint8_t PadIndexFromCoordQ8(uint16_t coordQ8) {
  return (uint8_t)(coordQ8 / kPadSpanQ8);
}

IRAM_ATTR bool IsAdjacentPad(uint8_t aPadX, uint8_t aPadY, uint8_t bPadX, uint8_t bPadY) {
  const int16_t dx = (int16_t)aPadX - (int16_t)bPadX;
  const int16_t dy = (int16_t)aPadY - (int16_t)bPadY;
  return ((dx == 0) && ((dy == 1) || (dy == -1))) || ((dy == 0) && ((dx == 1) || (dx == -1)));
}

IRAM_ATTR uint16_t LocalCoordFromQ8(uint16_t coordQ8) {
  return (uint16_t)(coordQ8 % kPadSpanQ8);
}

IRAM_ATTR bool NearLeftEdge(uint16_t xQ8) {
  return LocalCoordFromQ8(xQ8) <= kBoundaryLocalThresholdQ8;
}

IRAM_ATTR bool NearRightEdge(uint16_t xQ8) {
  return LocalCoordFromQ8(xQ8) >= (uint16_t)(kPadSpanQ8 - kBoundaryLocalThresholdQ8);
}

IRAM_ATTR bool NearTopEdge(uint16_t yQ8) {
  return LocalCoordFromQ8(yQ8) <= kBoundaryLocalThresholdQ8;
}

IRAM_ATTR bool NearBottomEdge(uint16_t yQ8) {
  return LocalCoordFromQ8(yQ8) >= (uint16_t)(kPadSpanQ8 - kBoundaryLocalThresholdQ8);
}

IRAM_ATTR bool IsBoundaryContinuation(uint16_t fromXQ8, uint16_t fromYQ8, uint16_t toXQ8, uint16_t toYQ8) {
  const uint8_t fromPadX = PadIndexFromCoordQ8(fromXQ8);
  const uint8_t fromPadY = PadIndexFromCoordQ8(fromYQ8);
  const uint8_t toPadX = PadIndexFromCoordQ8(toXQ8);
  const uint8_t toPadY = PadIndexFromCoordQ8(toYQ8);

  if ((fromPadY == toPadY) && (toPadX == (uint8_t)(fromPadX + 1U)))
  {
    return NearRightEdge(fromXQ8);
  }

  if ((fromPadY == toPadY) && (fromPadX == (uint8_t)(toPadX + 1U)))
  {
    return NearLeftEdge(fromXQ8);
  }

  if ((fromPadX == toPadX) && (toPadY == (uint8_t)(fromPadY + 1U)))
  {
    return NearBottomEdge(fromYQ8);
  }

  if ((fromPadX == toPadX) && (fromPadY == (uint8_t)(toPadY + 1U)))
  {
    return NearTopEdge(fromYQ8);
  }

  return false;
}

IRAM_ATTR bool HasBoundaryContinuationCandidate(uint16_t fromXQ8, uint16_t fromYQ8, const TouchPoint* detectedPoints,
                                                uint8_t detectedCount, const bool* detectedMatched, uint16_t touchOffThreshold) {
  for (uint8_t detectedIndex = 0U; detectedIndex < detectedCount; ++detectedIndex)
  {
    if (detectedMatched[detectedIndex] || (detectedPoints[detectedIndex].rawPressure < touchOffThreshold))
    {
      continue;
    }

    if (IsBoundaryContinuation(fromXQ8, fromYQ8, detectedPoints[detectedIndex].rawXQ8, detectedPoints[detectedIndex].rawYQ8))
    {
      return true;
    }
  }

  return false;
}

IRAM_ATTR bool TryAxisBiasedScore(uint16_t fromXQ8, uint16_t fromYQ8, uint16_t toXQ8, uint16_t toYQ8, uint32_t& scoreOut) {
  const uint8_t fromPadX = PadIndexFromCoordQ8(fromXQ8);
  const uint8_t fromPadY = PadIndexFromCoordQ8(fromYQ8);
  const uint8_t toPadX = PadIndexFromCoordQ8(toXQ8);
  const uint8_t toPadY = PadIndexFromCoordQ8(toYQ8);
  const uint16_t dxQ8 = AbsDiffU16(fromXQ8, toXQ8);
  const uint16_t dyQ8 = AbsDiffU16(fromYQ8, toYQ8);

  if ((fromPadX == toPadX) && (fromPadY == toPadY))
  {
    if ((dxQ8 > kSamePadAxisLimitQ8) || (dyQ8 > kSamePadAxisLimitQ8))
    {
      return false;
    }

    scoreOut = (uint32_t)dxQ8 + (uint32_t)dyQ8;
    return true;
  }

  if (!IsAdjacentPad(fromPadX, fromPadY, toPadX, toPadY))
  {
    return false;
  }

  if (fromPadX != toPadX)
  {
    if ((dxQ8 > kNeighborPrimaryAxisLimitQ8) || (dyQ8 > kNeighborSecondaryAxisLimitQ8))
    {
      return false;
    }

    scoreOut = (uint32_t)dxQ8 + (uint32_t)dyQ8 * 3U;
    if (IsBoundaryContinuation(fromXQ8, fromYQ8, toXQ8, toYQ8))
    {
      scoreOut /= 4U;
    }
    return true;
  }

  if ((dyQ8 > kNeighborPrimaryAxisLimitQ8) || (dxQ8 > kNeighborSecondaryAxisLimitQ8))
  {
    return false;
  }

  scoreOut = (uint32_t)dyQ8 + (uint32_t)dxQ8 * 3U;
  if (IsBoundaryContinuation(fromXQ8, fromYQ8, toXQ8, toYQ8))
  {
    scoreOut /= 4U;
  }
  return true;
}

IRAM_ATTR bool IsDuplicateCandidate(uint16_t candidateXQ8, uint16_t candidateYQ8) {
  const uint8_t candidatePadX = PadIndexFromCoordQ8(candidateXQ8);
  const uint8_t candidatePadY = PadIndexFromCoordQ8(candidateYQ8);

  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    const TouchPoint& tracked = touchPoints[index];
    if (!tracked.active || !tracked.matched)
    {
      continue;
    }

    const uint8_t trackedPadX = PadIndexFromCoordQ8(tracked.xQ8);
    const uint8_t trackedPadY = PadIndexFromCoordQ8(tracked.yQ8);
    if ((trackedPadX != candidatePadX || trackedPadY != candidatePadY) &&
        !IsAdjacentPad(trackedPadX, trackedPadY, candidatePadX, candidatePadY))
    {
      continue;
    }

    const uint16_t dxQ8 = AbsDiffU16(tracked.xQ8, candidateXQ8);
    const uint16_t dyQ8 = AbsDiffU16(tracked.yQ8, candidateYQ8);

    if ((dxQ8 <= kDuplicatePrimaryAxisLimitQ8) && (dyQ8 <= kDuplicateSecondaryAxisLimitQ8))
    {
      return true;
    }

    if ((dyQ8 <= kDuplicatePrimaryAxisLimitQ8) && (dxQ8 <= kDuplicateSecondaryAxisLimitQ8))
    {
      return true;
    }
  }

  return false;
}

IRAM_ATTR uint16_t SmoothU16(uint16_t previous, uint16_t current, uint16_t alphaX1000) {
  return (uint16_t)(((1000U - alphaX1000) * (uint32_t)previous + alphaX1000 * (uint32_t)current + 500U) / 1000U);
}

IRAM_ATTR uint16_t ScaleReading(uint16_t reading) {
  return (uint16_t)((reading << 4) + (reading >> 8)); // 12-bit to 16-bit
}

IRAM_ATTR uint16_t NormalizeTouchReading(uint16_t reading) {
  const uint16_t scaled = ScaleReading(reading);
  const uint16_t lowThreshold = (uint16_t)mpeConfig.lowThreshold;
  const uint16_t highThreshold = (uint16_t)mpeConfig.highThreshold;

  if (scaled <= lowThreshold)
  {
    return 0U;
  }

  if ((highThreshold <= lowThreshold) || (scaled >= highThreshold))
  {
    return UINT16_MAX;
  }

  const uint32_t numerator = ((uint32_t)(scaled - lowThreshold)) * UINT16_MAX;
  return (uint16_t)(numerator / (uint32_t)(highThreshold - lowThreshold));
}

IRAM_ATTR uint16_t TouchOnThreshold() {
  return kTouchOnThreshold;
}

IRAM_ATTR uint16_t TouchOffThreshold() {
  return (TouchOnThreshold() > kTouchHysteresis) ? (uint16_t)(TouchOnThreshold() - kTouchHysteresis) : 0U;
}

IRAM_ATTR uint16_t SmoothEMA(uint16_t input, uint16_t prev) {
  constexpr uint32_t alphaX1000 = 200U;
  return (uint16_t)(((1000U - alphaX1000) * (uint32_t)prev + alphaX1000 * (uint32_t)input + 500U) / 1000U);
}

IRAM_ATTR void ClearTouchPoints(TouchPoint* points) {
  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    points[index] = {};
  }
}

IRAM_ATTR uint8_t AllocateTrackingId() {
  const uint8_t id = nextTrackingId;
  nextTrackingId++;
  if (nextTrackingId == kInvalidTrackingId)
  {
    nextTrackingId = 0U;
  }
  return id;
}

IRAM_ATTR int8_t FindFreeTouchSlot() {
  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    if (!touchPoints[index].active)
    {
      return (int8_t)index;
    }
  }
  return -1;
}

IRAM_ATTR void InsertTouchPoint(TouchPoint* points, uint8_t& count, float x, float y, float pressure) {
  if (count >= kMaxTouchPoints)
  {
    uint8_t weakestIndex = 0U;
    uint16_t weakestPressure = points[0].rawPressure;

    for (uint8_t index = 1U; index < kMaxTouchPoints; ++index)
    {
      if (points[index].rawPressure < weakestPressure)
      {
        weakestPressure = points[index].rawPressure;
        weakestIndex = index;
      }
    }

    if (pressure <= weakestPressure)
    {
      return;
    }

    points[weakestIndex].rawXQ8 = QuantizeCoord(x);
    points[weakestIndex].rawYQ8 = QuantizeCoord(y);
    points[weakestIndex].rawPressure = (uint16_t)pressure;
    return;
  }

  points[count].rawXQ8 = QuantizeCoord(x);
  points[count].rawYQ8 = QuantizeCoord(y);
  points[count].rawPressure = (uint16_t)pressure;
  count++;
}

IRAM_ATTR bool BoxesShouldMerge(const TouchBlob& a, const TouchBlob& b) {
  if (!a.valid || !b.valid)
  {
    return false;
  }

  const bool xOverlaps = !((a.maxX + kBlobMergeGap) < b.minX || (b.maxX + kBlobMergeGap) < a.minX);
  const bool yOverlaps = !((a.maxY + kBlobMergeGap) < b.minY || (b.maxY + kBlobMergeGap) < a.minY);
  return xOverlaps && yOverlaps;
}

IRAM_ATTR uint8_t RangeGap(uint8_t aMin, uint8_t aMax, uint8_t bMin, uint8_t bMax) {
  if (aMax < bMin)
  {
    return (uint8_t)(bMin - aMax - 1U);
  }

  if (bMax < aMin)
  {
    return (uint8_t)(aMin - bMax - 1U);
  }

  return 0U;
}

IRAM_ATTR bool AxisAlignedShouldMerge(const TouchBlob& a, const TouchBlob& b) {
  if (!a.valid || !b.valid)
  {
    return false;
  }

  const uint8_t xGap = RangeGap(a.minX, a.maxX, b.minX, b.maxX);
  const uint8_t yGap = RangeGap(a.minY, a.maxY, b.minY, b.maxY);
  const uint8_t combinedSpanX = (a.maxX > b.maxX ? a.maxX : b.maxX) - (a.minX < b.minX ? a.minX : b.minX) + 1U;
  const uint8_t combinedSpanY = (a.maxY > b.maxY ? a.maxY : b.maxY) - (a.minY < b.minY ? a.minY : b.minY) + 1U;

  const bool horizontalMerge =
      (yGap <= kAxisMergeSecondaryGap) && (xGap <= kAxisMergePrimaryGap) && (combinedSpanX <= kAxisMergeCombinedSpan);
  const bool verticalMerge =
      (xGap <= kAxisMergeSecondaryGap) && (yGap <= kAxisMergePrimaryGap) && (combinedSpanY <= kAxisMergeCombinedSpan);

  return horizontalMerge || verticalMerge;
}

IRAM_ATTR void MergeBlobInto(TouchBlob& target, const TouchBlob& source) {
  if (!source.valid)
  {
    return;
  }

  if (!target.valid)
  {
    target = source;
    return;
  }

  const float targetWeight = target.pressure;
  const float sourceWeight = source.pressure;
  const float combinedWeight = targetWeight + sourceWeight;

  if (combinedWeight > 0.0f)
  {
    target.centerX = ((target.centerX * targetWeight) + (source.centerX * sourceWeight)) / combinedWeight;
    target.centerY = ((target.centerY * targetWeight) + (source.centerY * sourceWeight)) / combinedWeight;
  }
  target.pressure = (target.pressure > source.pressure) ? target.pressure : source.pressure;
  target.minX = (target.minX < source.minX) ? target.minX : source.minX;
  target.minY = (target.minY < source.minY) ? target.minY : source.minY;
  target.maxX = (target.maxX > source.maxX) ? target.maxX : source.maxX;
  target.maxY = (target.maxY > source.maxY) ? target.maxY : source.maxY;
  target.valid = true;
}

IRAM_ATTR void MergeNearbyBlobs(TouchBlob* blobs, uint8_t& blobCount) {
  if (blobCount < 2U)
  {
    return;
  }

  bool mergedAny = true;
  while (mergedAny)
  {
    mergedAny = false;

    for (uint8_t i = 0U; i < blobCount; ++i)
    {
      if (!blobs[i].valid)
      {
        continue;
      }

      for (uint8_t j = (uint8_t)(i + 1U); j < blobCount; ++j)
      {
        if (!blobs[j].valid || !(BoxesShouldMerge(blobs[i], blobs[j]) || AxisAlignedShouldMerge(blobs[i], blobs[j])))
        {
          continue;
        }

        MergeBlobInto(blobs[i], blobs[j]);
        blobs[j].valid = false;
        mergedAny = true;
      }
    }

    if (mergedAny)
    {
      uint8_t compactedCount = 0U;
      for (uint8_t index = 0U; index < blobCount; ++index)
      {
        if (blobs[index].valid)
        {
          blobs[compactedCount++] = blobs[index];
        }
      }
      for (uint8_t index = compactedCount; index < blobCount; ++index)
      {
        blobs[index] = {};
      }
      blobCount = compactedCount;
    }
  }
}

IRAM_ATTR void DetectTouchPoints(const MPECoprocessorLink::MPEData& mpeData, TouchPoint* detectedPoints, uint8_t& detectedCount,
                                 uint16_t detectThreshold) {
  uint8_t visited[kGridSize][kGridSize] = {};
  uint8_t queueX[kGridSize * kGridSize] = {};
  uint8_t queueY[kGridSize * kGridSize] = {};
  TouchBlob blobs[kMaxTouchPoints] = {};
  uint8_t blobCount = 0U;

  detectedCount = 0U;
  ClearTouchPoints(detectedPoints);

  for (uint8_t x = 0U; x < kGridSize; ++x)
  {
    for (uint8_t y = 0U; y < kGridSize; ++y)
    {
      const uint16_t seedReading = NormalizeTouchReading(mpeData[x][y]);
      if (visited[x][y] || (seedReading <= detectThreshold))
      {
        continue;
      }

      uint16_t head = 0U;
      uint16_t tail = 0U;
      uint32_t weightSum = 0U;
      uint64_t weightedXSum = 0U;
      uint64_t weightedYSum = 0U;
      uint16_t peak = 0U;
      uint8_t minBlobX = x;
      uint8_t minBlobY = y;
      uint8_t maxBlobX = x;
      uint8_t maxBlobY = y;

      visited[x][y] = 1U;
      queueX[tail] = x;
      queueY[tail] = y;
      tail++;

      while (head < tail)
      {
        const uint8_t currentX = queueX[head];
        const uint8_t currentY = queueY[head];
        const uint16_t normalizedReading = NormalizeTouchReading(mpeData[currentX][currentY]);
        const uint16_t weightedReading = ScaleReading(mpeData[currentX][currentY]);
        head++;

        if (normalizedReading <= detectThreshold)
        {
          continue;
        }

        weightSum += weightedReading;
        weightedXSum += (uint64_t)((uint32_t)currentX * 2U + 1U) * (uint64_t)weightedReading;
        weightedYSum += (uint64_t)((uint32_t)currentY * 2U + 1U) * (uint64_t)weightedReading;
        if (weightedReading > peak)
        {
          peak = weightedReading;
        }
        if (currentX < minBlobX)
        {
          minBlobX = currentX;
        }
        if (currentY < minBlobY)
        {
          minBlobY = currentY;
        }
        if (currentX > maxBlobX)
        {
          maxBlobX = currentX;
        }
        if (currentY > maxBlobY)
        {
          maxBlobY = currentY;
        }

        for (int8_t dx = -kTouchMergeRadius; dx <= kTouchMergeRadius; ++dx)
        {
          for (int8_t dy = -kTouchMergeRadius; dy <= kTouchMergeRadius; ++dy)
          {
            if ((dx == 0) && (dy == 0))
            {
              continue;
            }

            const int16_t nextX = (int16_t)currentX + dx;
            const int16_t nextY = (int16_t)currentY + dy;

            if ((nextX < 0) || (nextX >= kGridSize) || (nextY < 0) || (nextY >= kGridSize))
            {
              continue;
            }

            if (visited[nextX][nextY] || (NormalizeTouchReading(mpeData[nextX][nextY]) <= detectThreshold))
            {
              continue;
            }

            visited[nextX][nextY] = 1U;
            queueX[tail] = (uint8_t)nextX;
            queueY[tail] = (uint8_t)nextY;
            tail++;
          }
        }
      }

      if ((weightSum == 0U) || (peak == 0U))
      {
        continue;
      }

      if (blobCount < kMaxTouchPoints)
      {
        blobs[blobCount].centerX = (float)weightedXSum / (2.0f * (float)weightSum);
        blobs[blobCount].centerY = (float)weightedYSum / (2.0f * (float)weightSum);
        blobs[blobCount].pressure = (float)peak;
        blobs[blobCount].minX = minBlobX;
        blobs[blobCount].minY = minBlobY;
        blobs[blobCount].maxX = maxBlobX;
        blobs[blobCount].maxY = maxBlobY;
        blobs[blobCount].valid = true;
        blobCount++;
      }
    }
  }

  MergeNearbyBlobs(blobs, blobCount);

  for (uint8_t blobIndex = 0U; blobIndex < blobCount; ++blobIndex)
  {
    if (!blobs[blobIndex].valid)
    {
      continue;
    }

    InsertTouchPoint(detectedPoints, detectedCount, blobs[blobIndex].centerX, blobs[blobIndex].centerY, blobs[blobIndex].pressure);
  }
}

IRAM_ATTR void TrackTouchPoints(const TouchPoint* detectedPoints, uint8_t detectedCount, uint32_t nowMs) {
  const uint16_t touch_on_threshold = TouchOnThreshold();
  const uint16_t touchOffThreshold = TouchOffThreshold();
  bool detectedMatched[kMaxTouchPoints] = {};

  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    touchPoints[index].matched = false;
    touchPoints[index].justStarted = false;
    touchPoints[index].justEnded = false;
  }

  for (uint8_t trackedIndex = 0U; trackedIndex < kMaxTouchPoints; ++trackedIndex)
  {
    TouchPoint& tracked = touchPoints[trackedIndex];
    if (!tracked.active)
    {
      continue;
    }

    int8_t bestDetectedIndex = -1;
    uint32_t bestScore = 0U;
    const bool hasBoundaryContinuation = HasBoundaryContinuationCandidate(tracked.xQ8, tracked.yQ8, detectedPoints, detectedCount,
                                                                            detectedMatched, touchOffThreshold);

    for (uint8_t detectedIndex = 0U; detectedIndex < detectedCount; ++detectedIndex)
    {
      if (detectedMatched[detectedIndex] || (detectedPoints[detectedIndex].rawPressure < touchOffThreshold))
      {
        continue;
      }

      if (hasBoundaryContinuation &&
          (PadIndexFromCoordQ8(tracked.xQ8) == PadIndexFromCoordQ8(detectedPoints[detectedIndex].rawXQ8)) &&
          (PadIndexFromCoordQ8(tracked.yQ8) == PadIndexFromCoordQ8(detectedPoints[detectedIndex].rawYQ8)))
      {
        continue;
      }

      uint32_t axisScore = 0U;
      if (!TryAxisBiasedScore(tracked.xQ8, tracked.yQ8, detectedPoints[detectedIndex].rawXQ8,
                              detectedPoints[detectedIndex].rawYQ8, axisScore))
      {
        continue;
      }

      const uint32_t pressurePenalty = (tracked.pressure > detectedPoints[detectedIndex].rawPressure)
                                            ? (tracked.pressure - detectedPoints[detectedIndex].rawPressure)
                                            : (detectedPoints[detectedIndex].rawPressure - tracked.pressure);
      const uint32_t score = axisScore * 16U + pressurePenalty;

      if ((bestDetectedIndex < 0) || (score < bestScore))
      {
        bestDetectedIndex = (int8_t)detectedIndex;
        bestScore = score;
      }
    }

    if (bestDetectedIndex >= 0)
    {
      const TouchPoint& detected = detectedPoints[bestDetectedIndex];
      detectedMatched[bestDetectedIndex] = true;
      tracked.rawXQ8 = detected.rawXQ8;
      tracked.rawYQ8 = detected.rawYQ8;
      tracked.rawPressure = detected.rawPressure;
      tracked.xQ8 = SmoothU16(tracked.xQ8, detected.rawXQ8, kPositionAlphaX1000);
      tracked.yQ8 = SmoothU16(tracked.yQ8, detected.rawYQ8, kPositionAlphaX1000);
      tracked.pressure = SmoothU16(tracked.pressure, detected.rawPressure, kPressureAlphaX1000);
      tracked.matched = true;
      tracked.dirty = true;
      tracked.missingFrames = 0U;
      tracked.lastSeenMs = nowMs;
      continue;
    }

    if (tracked.missingFrames < UINT8_MAX)
    {
      tracked.missingFrames++;
    }

    if ((tracked.missingFrames > kMaxMissingFrames) && ((uint32_t)(nowMs - tracked.lastSeenMs) >= (uint32_t)(kTouchTimeoutUs / 1000U)))
    {
      tracked.justEnded = true;
      tracked.dirty = true;
      tracked.active = false;
    }
  }

  for (uint8_t detectedIndex = 0U; detectedIndex < detectedCount; ++detectedIndex)
  {
    if (detectedMatched[detectedIndex] || (detectedPoints[detectedIndex].rawPressure < touch_on_threshold))
    {
      continue;
    }

    if (IsDuplicateCandidate(detectedPoints[detectedIndex].rawXQ8, detectedPoints[detectedIndex].rawYQ8))
    {
      continue;
    }

    const int8_t freeSlot = FindFreeTouchSlot();
    if (freeSlot < 0)
    {
      continue;
    }

    TouchPoint& tracked = touchPoints[freeSlot];
    tracked = {};
    tracked.trackingId = AllocateTrackingId();
    tracked.xQ8 = detectedPoints[detectedIndex].rawXQ8;
    tracked.yQ8 = detectedPoints[detectedIndex].rawYQ8;
    tracked.pressure = detectedPoints[detectedIndex].rawPressure;
    tracked.rawXQ8 = detectedPoints[detectedIndex].rawXQ8;
    tracked.rawYQ8 = detectedPoints[detectedIndex].rawYQ8;
    tracked.rawPressure = detectedPoints[detectedIndex].rawPressure;
    tracked.active = true;
    tracked.matched = true;
    tracked.justStarted = true;
    tracked.dirty = true;
    tracked.missingFrames = 0U;
    tracked.lastSeenMs = nowMs;
  }
}

IRAM_ATTR bool TouchPointsChanged() {
  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    const TouchPoint& current = touchPoints[index];
    if (current.dirty || current.justEnded)
    {
      return true;
    }
  }

  return false;
}

void LogTouchPoints() {
  if (!TouchPointsChanged())
  {
    return;
  }

  char line[256];
  int written = snprintf(line, sizeof(line), "touch %.0fHz", scanRateHz);

  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    const TouchPoint& point = touchPoints[index];

    if (point.active)
    {
      const int appended = snprintf(line + written, sizeof(line) - (size_t)written, " ID%02u %.2f-%.2f (%.0f)", (unsigned)point.trackingId,
                                    DequantizeCoord(point.xQ8) / 3.0f, DequantizeCoord(point.yQ8) / 3.0f, (float)point.pressure);
      if ((appended <= 0) || ((size_t)(written + appended) >= sizeof(line)))
      {
        written = (int)sizeof(line) - 1;
        break;
      }
      written += appended;
    }
    else if (point.justEnded)
    {
      const int appended = snprintf(line + written, sizeof(line) - (size_t)written, " ID%02u UP", (unsigned)point.trackingId);
      if ((appended <= 0) || ((size_t)(written + appended) >= sizeof(line)))
      {
        written = (int)sizeof(line) - 1;
        break;
      }
      written += appended;
    }
  }

  ESP_LOGI(kTag, "%s", line);

  for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
  {
    touchPoints[index].dirty = false;
    touchPoints[index].justEnded = false;
  }
}

void LogRawFrame(const MPECoprocessorLink::MPEData& mpeData, uint64_t nowUs) {
  char line[512];

  for (uint8_t y = 0U; y < kGridSize; ++y)
  {
    int written = snprintf(line, sizeof(line), "mpe %llu %.0fHz y%u", (unsigned long long)nowUs, scanRateHz, (unsigned)y);

    for (uint8_t x = 0U; x < kGridSize; ++x)
    {
      const int appended = snprintf(line + written, sizeof(line) - (size_t)written, " %u", (unsigned)mpeData[x][y]);
      if ((appended <= 0) || ((size_t)(written + appended) >= sizeof(line)))
      {
        break;
      }
      written += appended;
    }
    ESP_LOGI(kTag, "%s", line);
  }
}
} // namespace

void Init() {
  coprocessorLink.Init();
}

void Start() {
  coprocessorLink.Start();
}

IRAM_ATTR bool Scan() {
  const auto& mpeData = coprocessorLink.GetMPEData();
  KeypadConfig config = mpeConfig;
  const uint64_t nowUs = esp_timer_get_time();

  if (lastScanUs != 0U)
  {
    const uint64_t deltaUs = nowUs - lastScanUs;
    if (deltaUs > 0U)
    {
      const float instantScanRateHz = 1000000.0f / (float)deltaUs;
      if (scanRateHz == 0.0f)
      {
        scanRateHz = instantScanRateHz;
      }
      else
      {
        scanRateHz = scanRateHz + (instantScanRateHz - scanRateHz) * 0.15f;
      }
    }
  }
  lastScanUs = nowUs;

  for (uint8_t y = 0U; y < Y_SIZE; ++y)
  {
    for (uint8_t x = 0U; x < X_SIZE; ++x)
    {
      const uint8_t baseX = (uint8_t)(x * MPECoprocessorLink::SECTOR_GRID_SIZE);
      const uint8_t baseY = (uint8_t)(y * MPECoprocessorLink::SECTOR_GRID_SIZE);
      uint16_t maxReading = 0U;

      for (uint8_t localY = 0U; localY < MPECoprocessorLink::SECTOR_GRID_SIZE; ++localY)
      {
        for (uint8_t localX = 0U; localX < MPECoprocessorLink::SECTOR_GRID_SIZE; ++localX)
        {
          const uint16_t reading = mpeData[baseX + localX][baseY + localY];
          if (reading > maxReading)
          {
            maxReading = reading;
          }
        }
      }

      maxReading = ScaleReading(maxReading);
      padForce[x][y] = SmoothEMA(maxReading, padForce[x][y]);

      if (keypadState[x][y].Update(config, (Fract16)padForce[x][y]))
      {
        if (NotifyOS(InputId{1, (uint16_t)(y * X_SIZE + x)}, &keypadState[x][y]))
        {
          return true;
        }
      }
    }
  }

  // LogRawFrame(mpeData, nowUs);

  return false;
}
} // namespace Device::KeyPad::MPE

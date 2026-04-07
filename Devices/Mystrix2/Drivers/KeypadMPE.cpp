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
    struct TouchPoint
    {
      uint8_t tracking_id = 0xFF;
      uint16_t x_q8 = 0U;
      uint16_t y_q8 = 0U;
      uint16_t pressure = 0U;
      uint16_t raw_x_q8 = 0U;
      uint16_t raw_y_q8 = 0U;
      uint16_t raw_pressure = 0U;
      bool active = false;
      bool matched = false;
      bool just_started = false;
      bool just_ended = false;
      bool dirty = false;
      uint8_t missing_frames = 0U;
      uint32_t last_seen_ms = 0U;
    };

    struct TouchBlob
    {
      float center_x = 0.0f;
      float center_y = 0.0f;
      float pressure = 0.0f;
      uint8_t min_x = 0U;
      uint8_t min_y = 0U;
      uint8_t max_x = 0U;
      uint8_t max_y = 0U;
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
    MPECoprocessorLink coprocessor_link;
    TouchPoint* touch_points = nullptr;
    uint64_t last_scan_us = 0U;
    uint8_t next_tracking_id = 0U;
    float scan_rate_hz = 0.0f;

    IRAM_ATTR uint16_t QuantizeCoord(float value)
    {
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

    IRAM_ATTR float DequantizeCoord(uint16_t value_q8)
    {
      return (float)value_q8 / 256.0f;
    }

    IRAM_ATTR uint16_t AbsDiffU16(uint16_t a, uint16_t b)
    {
      return (a >= b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
    }

    IRAM_ATTR uint8_t PadIndexFromCoordQ8(uint16_t coord_q8)
    {
      return (uint8_t)(coord_q8 / kPadSpanQ8);
    }

    IRAM_ATTR bool IsAdjacentPad(uint8_t a_pad_x, uint8_t a_pad_y, uint8_t b_pad_x, uint8_t b_pad_y)
    {
      const int16_t dx = (int16_t)a_pad_x - (int16_t)b_pad_x;
      const int16_t dy = (int16_t)a_pad_y - (int16_t)b_pad_y;
      return ((dx == 0) && ((dy == 1) || (dy == -1))) || ((dy == 0) && ((dx == 1) || (dx == -1)));
    }

    IRAM_ATTR uint16_t LocalCoordFromQ8(uint16_t coord_q8)
    {
      return (uint16_t)(coord_q8 % kPadSpanQ8);
    }

    IRAM_ATTR bool NearLeftEdge(uint16_t x_q8)
    {
      return LocalCoordFromQ8(x_q8) <= kBoundaryLocalThresholdQ8;
    }

    IRAM_ATTR bool NearRightEdge(uint16_t x_q8)
    {
      return LocalCoordFromQ8(x_q8) >= (uint16_t)(kPadSpanQ8 - kBoundaryLocalThresholdQ8);
    }

    IRAM_ATTR bool NearTopEdge(uint16_t y_q8)
    {
      return LocalCoordFromQ8(y_q8) <= kBoundaryLocalThresholdQ8;
    }

    IRAM_ATTR bool NearBottomEdge(uint16_t y_q8)
    {
      return LocalCoordFromQ8(y_q8) >= (uint16_t)(kPadSpanQ8 - kBoundaryLocalThresholdQ8);
    }

    IRAM_ATTR bool IsBoundaryContinuation(uint16_t from_x_q8,
                                          uint16_t from_y_q8,
                                          uint16_t to_x_q8,
                                          uint16_t to_y_q8)
    {
      const uint8_t from_pad_x = PadIndexFromCoordQ8(from_x_q8);
      const uint8_t from_pad_y = PadIndexFromCoordQ8(from_y_q8);
      const uint8_t to_pad_x = PadIndexFromCoordQ8(to_x_q8);
      const uint8_t to_pad_y = PadIndexFromCoordQ8(to_y_q8);

      if ((from_pad_y == to_pad_y) && (to_pad_x == (uint8_t)(from_pad_x + 1U)))
      {
        return NearRightEdge(from_x_q8);
      }

      if ((from_pad_y == to_pad_y) && (from_pad_x == (uint8_t)(to_pad_x + 1U)))
      {
        return NearLeftEdge(from_x_q8);
      }

      if ((from_pad_x == to_pad_x) && (to_pad_y == (uint8_t)(from_pad_y + 1U)))
      {
        return NearBottomEdge(from_y_q8);
      }

      if ((from_pad_x == to_pad_x) && (from_pad_y == (uint8_t)(to_pad_y + 1U)))
      {
        return NearTopEdge(from_y_q8);
      }

      return false;
    }

    IRAM_ATTR bool HasBoundaryContinuationCandidate(uint16_t from_x_q8,
                                                    uint16_t from_y_q8,
                                                    const TouchPoint* detected_points,
                                                    uint8_t detected_count,
                                                    const bool* detected_matched,
                                                    uint16_t touch_off_threshold)
    {
      for (uint8_t detected_index = 0U; detected_index < detected_count; ++detected_index)
      {
        if (detected_matched[detected_index] || (detected_points[detected_index].raw_pressure < touch_off_threshold))
        {
          continue;
        }

        if (IsBoundaryContinuation(from_x_q8,
                                   from_y_q8,
                                   detected_points[detected_index].raw_x_q8,
                                   detected_points[detected_index].raw_y_q8))
        {
          return true;
        }
      }

      return false;
    }

    IRAM_ATTR bool TryAxisBiasedScore(uint16_t from_x_q8,
                                      uint16_t from_y_q8,
                                      uint16_t to_x_q8,
                                      uint16_t to_y_q8,
                                      uint32_t& score_out)
    {
      const uint8_t from_pad_x = PadIndexFromCoordQ8(from_x_q8);
      const uint8_t from_pad_y = PadIndexFromCoordQ8(from_y_q8);
      const uint8_t to_pad_x = PadIndexFromCoordQ8(to_x_q8);
      const uint8_t to_pad_y = PadIndexFromCoordQ8(to_y_q8);
      const uint16_t dx_q8 = AbsDiffU16(from_x_q8, to_x_q8);
      const uint16_t dy_q8 = AbsDiffU16(from_y_q8, to_y_q8);

      if ((from_pad_x == to_pad_x) && (from_pad_y == to_pad_y))
      {
        if ((dx_q8 > kSamePadAxisLimitQ8) || (dy_q8 > kSamePadAxisLimitQ8))
        {
          return false;
        }

        score_out = (uint32_t)dx_q8 + (uint32_t)dy_q8;
        return true;
      }

      if (!IsAdjacentPad(from_pad_x, from_pad_y, to_pad_x, to_pad_y))
      {
        return false;
      }

      if (from_pad_x != to_pad_x)
      {
        if ((dx_q8 > kNeighborPrimaryAxisLimitQ8) || (dy_q8 > kNeighborSecondaryAxisLimitQ8))
        {
          return false;
        }

        score_out = (uint32_t)dx_q8 + (uint32_t)dy_q8 * 3U;
        if (IsBoundaryContinuation(from_x_q8, from_y_q8, to_x_q8, to_y_q8))
        {
          score_out /= 4U;
        }
        return true;
      }

      if ((dy_q8 > kNeighborPrimaryAxisLimitQ8) || (dx_q8 > kNeighborSecondaryAxisLimitQ8))
      {
        return false;
      }

      score_out = (uint32_t)dy_q8 + (uint32_t)dx_q8 * 3U;
      if (IsBoundaryContinuation(from_x_q8, from_y_q8, to_x_q8, to_y_q8))
      {
        score_out /= 4U;
      }
      return true;
    }

    IRAM_ATTR bool IsDuplicateCandidate(uint16_t candidate_x_q8, uint16_t candidate_y_q8)
    {
      const uint8_t candidate_pad_x = PadIndexFromCoordQ8(candidate_x_q8);
      const uint8_t candidate_pad_y = PadIndexFromCoordQ8(candidate_y_q8);

      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        const TouchPoint& tracked = touch_points[index];
        if (!tracked.active || !tracked.matched)
        {
          continue;
        }

        const uint8_t tracked_pad_x = PadIndexFromCoordQ8(tracked.x_q8);
        const uint8_t tracked_pad_y = PadIndexFromCoordQ8(tracked.y_q8);
        if ((tracked_pad_x != candidate_pad_x || tracked_pad_y != candidate_pad_y) &&
            !IsAdjacentPad(tracked_pad_x, tracked_pad_y, candidate_pad_x, candidate_pad_y))
        {
          continue;
        }

        const uint16_t dx_q8 = AbsDiffU16(tracked.x_q8, candidate_x_q8);
        const uint16_t dy_q8 = AbsDiffU16(tracked.y_q8, candidate_y_q8);

        if ((dx_q8 <= kDuplicatePrimaryAxisLimitQ8) && (dy_q8 <= kDuplicateSecondaryAxisLimitQ8))
        {
          return true;
        }

        if ((dy_q8 <= kDuplicatePrimaryAxisLimitQ8) && (dx_q8 <= kDuplicateSecondaryAxisLimitQ8))
        {
          return true;
        }
      }

      return false;
    }

    IRAM_ATTR uint16_t SmoothU16(uint16_t previous, uint16_t current, uint16_t alpha_x1000)
    {
      return (uint16_t)(((1000U - alpha_x1000) * (uint32_t)previous + alpha_x1000 * (uint32_t)current + 500U) / 1000U);
    }

    IRAM_ATTR uint16_t ScaleReading(uint16_t reading)
    {
      return (uint16_t)((reading << 4) + (reading >> 8));  // 12-bit to 16-bit
    }

    IRAM_ATTR uint16_t NormalizeTouchReading(uint16_t reading)
    {
      const uint16_t scaled = ScaleReading(reading);
      const uint16_t low_threshold = (uint16_t)mpe_config.low_threshold;
      const uint16_t high_threshold = (uint16_t)mpe_config.high_threshold;

      if (scaled <= low_threshold)
      {
        return 0U;
      }

      if ((high_threshold <= low_threshold) || (scaled >= high_threshold))
      {
        return UINT16_MAX;
      }

      const uint32_t numerator = ((uint32_t)(scaled - low_threshold)) * UINT16_MAX;
      return (uint16_t)(numerator / (uint32_t)(high_threshold - low_threshold));
    }

    IRAM_ATTR uint16_t TouchOnThreshold()
    {
      return kTouchOnThreshold;
    }

    IRAM_ATTR uint16_t TouchOffThreshold()
    {
      return (TouchOnThreshold() > kTouchHysteresis) ? (uint16_t)(TouchOnThreshold() - kTouchHysteresis) : 0U;
    }

    IRAM_ATTR uint16_t SmoothEMA(uint16_t input, uint16_t prev)
    {
      constexpr uint32_t alpha_x1000 = 200U;
      return (uint16_t)(((1000U - alpha_x1000) * (uint32_t)prev + alpha_x1000 * (uint32_t)input + 500U) / 1000U);
    }

    IRAM_ATTR void ClearTouchPoints(TouchPoint* points)
    {
      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        points[index] = {};
      }
    }

    IRAM_ATTR uint8_t AllocateTrackingId()
    {
      const uint8_t id = next_tracking_id;
      next_tracking_id++;
      if (next_tracking_id == kInvalidTrackingId)
      {
        next_tracking_id = 0U;
      }
      return id;
    }

    IRAM_ATTR int8_t FindFreeTouchSlot()
    {
      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        if (!touch_points[index].active)
        {
          return (int8_t)index;
        }
      }
      return -1;
    }

    IRAM_ATTR void InsertTouchPoint(TouchPoint* points, uint8_t& count, float x, float y, float pressure)
    {
      if (count >= kMaxTouchPoints)
      {
        uint8_t weakest_index = 0U;
        uint16_t weakest_pressure = points[0].raw_pressure;

        for (uint8_t index = 1U; index < kMaxTouchPoints; ++index)
        {
          if (points[index].raw_pressure < weakest_pressure)
          {
            weakest_pressure = points[index].raw_pressure;
            weakest_index = index;
          }
        }

        if (pressure <= weakest_pressure)
        {
          return;
        }

        points[weakest_index].raw_x_q8 = QuantizeCoord(x);
        points[weakest_index].raw_y_q8 = QuantizeCoord(y);
        points[weakest_index].raw_pressure = (uint16_t)pressure;
        return;
      }

      points[count].raw_x_q8 = QuantizeCoord(x);
      points[count].raw_y_q8 = QuantizeCoord(y);
      points[count].raw_pressure = (uint16_t)pressure;
      count++;
    }

    IRAM_ATTR bool BoxesShouldMerge(const TouchBlob& a, const TouchBlob& b)
    {
      if (!a.valid || !b.valid)
      {
        return false;
      }

      const bool x_overlaps = !((a.max_x + kBlobMergeGap) < b.min_x || (b.max_x + kBlobMergeGap) < a.min_x);
      const bool y_overlaps = !((a.max_y + kBlobMergeGap) < b.min_y || (b.max_y + kBlobMergeGap) < a.min_y);
      return x_overlaps && y_overlaps;
    }

    IRAM_ATTR uint8_t RangeGap(uint8_t a_min, uint8_t a_max, uint8_t b_min, uint8_t b_max)
    {
      if (a_max < b_min)
      {
        return (uint8_t)(b_min - a_max - 1U);
      }

      if (b_max < a_min)
      {
        return (uint8_t)(a_min - b_max - 1U);
      }

      return 0U;
    }

    IRAM_ATTR bool AxisAlignedShouldMerge(const TouchBlob& a, const TouchBlob& b)
    {
      if (!a.valid || !b.valid)
      {
        return false;
      }

      const uint8_t x_gap = RangeGap(a.min_x, a.max_x, b.min_x, b.max_x);
      const uint8_t y_gap = RangeGap(a.min_y, a.max_y, b.min_y, b.max_y);
      const uint8_t combined_span_x =
          (a.max_x > b.max_x ? a.max_x : b.max_x) - (a.min_x < b.min_x ? a.min_x : b.min_x) + 1U;
      const uint8_t combined_span_y =
          (a.max_y > b.max_y ? a.max_y : b.max_y) - (a.min_y < b.min_y ? a.min_y : b.min_y) + 1U;

      const bool horizontal_merge =
          (y_gap <= kAxisMergeSecondaryGap) && (x_gap <= kAxisMergePrimaryGap) && (combined_span_x <= kAxisMergeCombinedSpan);
      const bool vertical_merge =
          (x_gap <= kAxisMergeSecondaryGap) && (y_gap <= kAxisMergePrimaryGap) && (combined_span_y <= kAxisMergeCombinedSpan);

      return horizontal_merge || vertical_merge;
    }

    IRAM_ATTR void MergeBlobInto(TouchBlob& target, const TouchBlob& source)
    {
      if (!source.valid)
      {
        return;
      }

      if (!target.valid)
      {
        target = source;
        return;
      }

      const float target_weight = target.pressure;
      const float source_weight = source.pressure;
      const float combined_weight = target_weight + source_weight;

      if (combined_weight > 0.0f)
      {
        target.center_x = ((target.center_x * target_weight) + (source.center_x * source_weight)) / combined_weight;
        target.center_y = ((target.center_y * target_weight) + (source.center_y * source_weight)) / combined_weight;
      }
      target.pressure = (target.pressure > source.pressure) ? target.pressure : source.pressure;
      target.min_x = (target.min_x < source.min_x) ? target.min_x : source.min_x;
      target.min_y = (target.min_y < source.min_y) ? target.min_y : source.min_y;
      target.max_x = (target.max_x > source.max_x) ? target.max_x : source.max_x;
      target.max_y = (target.max_y > source.max_y) ? target.max_y : source.max_y;
      target.valid = true;
    }

    IRAM_ATTR void MergeNearbyBlobs(TouchBlob* blobs, uint8_t& blob_count)
    {
      if (blob_count < 2U)
      {
        return;
      }

      bool merged_any = true;
      while (merged_any)
      {
        merged_any = false;

        for (uint8_t i = 0U; i < blob_count; ++i)
        {
          if (!blobs[i].valid)
          {
            continue;
          }

          for (uint8_t j = (uint8_t)(i + 1U); j < blob_count; ++j)
          {
            if (!blobs[j].valid || !(BoxesShouldMerge(blobs[i], blobs[j]) || AxisAlignedShouldMerge(blobs[i], blobs[j])))
            {
              continue;
            }

            MergeBlobInto(blobs[i], blobs[j]);
            blobs[j].valid = false;
            merged_any = true;
          }
        }

        if (merged_any)
        {
          uint8_t compacted_count = 0U;
          for (uint8_t index = 0U; index < blob_count; ++index)
          {
            if (blobs[index].valid)
            {
              blobs[compacted_count++] = blobs[index];
            }
          }
          for (uint8_t index = compacted_count; index < blob_count; ++index)
          {
            blobs[index] = {};
          }
          blob_count = compacted_count;
        }
      }
    }

    IRAM_ATTR void DetectTouchPoints(const MPECoprocessorLink::MPEData& mpe_data,
                                     TouchPoint* detected_points,
                                     uint8_t& detected_count,
                                     uint16_t detect_threshold)
    {
      uint8_t visited[kGridSize][kGridSize] = {};
      uint8_t queue_x[kGridSize * kGridSize] = {};
      uint8_t queue_y[kGridSize * kGridSize] = {};
      TouchBlob blobs[kMaxTouchPoints] = {};
      uint8_t blob_count = 0U;

      detected_count = 0U;
      ClearTouchPoints(detected_points);

      for (uint8_t x = 0U; x < kGridSize; ++x)
      {
        for (uint8_t y = 0U; y < kGridSize; ++y)
        {
          const uint16_t seed_reading = NormalizeTouchReading(mpe_data[x][y]);
          if (visited[x][y] || (seed_reading <= detect_threshold))
          {
            continue;
          }

          uint16_t head = 0U;
          uint16_t tail = 0U;
          uint32_t weight_sum = 0U;
          uint64_t weighted_x_sum = 0U;
          uint64_t weighted_y_sum = 0U;
          uint16_t peak = 0U;
          uint8_t min_blob_x = x;
          uint8_t min_blob_y = y;
          uint8_t max_blob_x = x;
          uint8_t max_blob_y = y;

          visited[x][y] = 1U;
          queue_x[tail] = x;
          queue_y[tail] = y;
          tail++;

          while (head < tail)
          {
            const uint8_t current_x = queue_x[head];
            const uint8_t current_y = queue_y[head];
            const uint16_t normalized_reading = NormalizeTouchReading(mpe_data[current_x][current_y]);
            const uint16_t weighted_reading = ScaleReading(mpe_data[current_x][current_y]);
            head++;

            if (normalized_reading <= detect_threshold)
            {
              continue;
            }

            weight_sum += weighted_reading;
            weighted_x_sum += (uint64_t)((uint32_t)current_x * 2U + 1U) * (uint64_t)weighted_reading;
            weighted_y_sum += (uint64_t)((uint32_t)current_y * 2U + 1U) * (uint64_t)weighted_reading;
            if (weighted_reading > peak)
            {
              peak = weighted_reading;
            }
            if (current_x < min_blob_x) { min_blob_x = current_x; }
            if (current_y < min_blob_y) { min_blob_y = current_y; }
            if (current_x > max_blob_x) { max_blob_x = current_x; }
            if (current_y > max_blob_y) { max_blob_y = current_y; }

            for (int8_t dx = -kTouchMergeRadius; dx <= kTouchMergeRadius; ++dx)
            {
              for (int8_t dy = -kTouchMergeRadius; dy <= kTouchMergeRadius; ++dy)
              {
                if ((dx == 0) && (dy == 0))
                {
                  continue;
                }

                const int16_t next_x = (int16_t)current_x + dx;
                const int16_t next_y = (int16_t)current_y + dy;

                if ((next_x < 0) || (next_x >= kGridSize) || (next_y < 0) || (next_y >= kGridSize))
                {
                  continue;
                }

                if (visited[next_x][next_y] || (NormalizeTouchReading(mpe_data[next_x][next_y]) <= detect_threshold))
                {
                  continue;
                }

                visited[next_x][next_y] = 1U;
                queue_x[tail] = (uint8_t)next_x;
                queue_y[tail] = (uint8_t)next_y;
                tail++;
              }
            }
          }

          if ((weight_sum == 0U) || (peak == 0U))
          {
            continue;
          }

          if (blob_count < kMaxTouchPoints)
          {
            blobs[blob_count].center_x = (float)weighted_x_sum / (2.0f * (float)weight_sum);
            blobs[blob_count].center_y = (float)weighted_y_sum / (2.0f * (float)weight_sum);
            blobs[blob_count].pressure = (float)peak;
            blobs[blob_count].min_x = min_blob_x;
            blobs[blob_count].min_y = min_blob_y;
            blobs[blob_count].max_x = max_blob_x;
            blobs[blob_count].max_y = max_blob_y;
            blobs[blob_count].valid = true;
            blob_count++;
          }
        }
      }

      MergeNearbyBlobs(blobs, blob_count);

      for (uint8_t blob_index = 0U; blob_index < blob_count; ++blob_index)
      {
        if (!blobs[blob_index].valid)
        {
          continue;
        }

        InsertTouchPoint(detected_points,
                         detected_count,
                         blobs[blob_index].center_x,
                         blobs[blob_index].center_y,
                         blobs[blob_index].pressure);
      }
    }

    IRAM_ATTR void TrackTouchPoints(const TouchPoint* detected_points, uint8_t detected_count, uint32_t now_ms)
    {
      const uint16_t touch_on_threshold = TouchOnThreshold();
      const uint16_t touch_off_threshold = TouchOffThreshold();
      bool detected_matched[kMaxTouchPoints] = {};

      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        touch_points[index].matched = false;
        touch_points[index].just_started = false;
        touch_points[index].just_ended = false;
      }

      for (uint8_t tracked_index = 0U; tracked_index < kMaxTouchPoints; ++tracked_index)
      {
        TouchPoint& tracked = touch_points[tracked_index];
        if (!tracked.active)
        {
          continue;
        }

        int8_t best_detected_index = -1;
        uint32_t best_score = 0U;
        const bool has_boundary_continuation =
            HasBoundaryContinuationCandidate(tracked.x_q8, tracked.y_q8, detected_points, detected_count, detected_matched, touch_off_threshold);

        for (uint8_t detected_index = 0U; detected_index < detected_count; ++detected_index)
        {
          if (detected_matched[detected_index] || (detected_points[detected_index].raw_pressure < touch_off_threshold))
          {
            continue;
          }

          if (has_boundary_continuation &&
              (PadIndexFromCoordQ8(tracked.x_q8) == PadIndexFromCoordQ8(detected_points[detected_index].raw_x_q8)) &&
              (PadIndexFromCoordQ8(tracked.y_q8) == PadIndexFromCoordQ8(detected_points[detected_index].raw_y_q8)))
          {
            continue;
          }

          uint32_t axis_score = 0U;
          if (!TryAxisBiasedScore(tracked.x_q8,
                                  tracked.y_q8,
                                  detected_points[detected_index].raw_x_q8,
                                  detected_points[detected_index].raw_y_q8,
                                  axis_score))
          {
            continue;
          }

          const uint32_t pressure_penalty =
              (tracked.pressure > detected_points[detected_index].raw_pressure)
                  ? (tracked.pressure - detected_points[detected_index].raw_pressure)
                  : (detected_points[detected_index].raw_pressure - tracked.pressure);
          const uint32_t score = axis_score * 16U + pressure_penalty;

          if ((best_detected_index < 0) || (score < best_score))
          {
            best_detected_index = (int8_t)detected_index;
            best_score = score;
          }
        }

        if (best_detected_index >= 0)
        {
          const TouchPoint& detected = detected_points[best_detected_index];
          detected_matched[best_detected_index] = true;
          tracked.raw_x_q8 = detected.raw_x_q8;
          tracked.raw_y_q8 = detected.raw_y_q8;
          tracked.raw_pressure = detected.raw_pressure;
          tracked.x_q8 = SmoothU16(tracked.x_q8, detected.raw_x_q8, kPositionAlphaX1000);
          tracked.y_q8 = SmoothU16(tracked.y_q8, detected.raw_y_q8, kPositionAlphaX1000);
          tracked.pressure = SmoothU16(tracked.pressure, detected.raw_pressure, kPressureAlphaX1000);
          tracked.matched = true;
          tracked.dirty = true;
          tracked.missing_frames = 0U;
          tracked.last_seen_ms = now_ms;
          continue;
        }

        if (tracked.missing_frames < UINT8_MAX)
        {
          tracked.missing_frames++;
        }

        if ((tracked.missing_frames > kMaxMissingFrames) &&
            ((uint32_t)(now_ms - tracked.last_seen_ms) >= (uint32_t)(kTouchTimeoutUs / 1000U)))
        {
          tracked.just_ended = true;
          tracked.dirty = true;
          tracked.active = false;
        }
      }

      for (uint8_t detected_index = 0U; detected_index < detected_count; ++detected_index)
      {
        if (detected_matched[detected_index] || (detected_points[detected_index].raw_pressure < touch_on_threshold))
        {
          continue;
        }

        if (IsDuplicateCandidate(detected_points[detected_index].raw_x_q8, detected_points[detected_index].raw_y_q8))
        {
          continue;
        }

        const int8_t free_slot = FindFreeTouchSlot();
        if (free_slot < 0)
        {
          continue;
        }

        TouchPoint& tracked = touch_points[free_slot];
        tracked = {};
        tracked.tracking_id = AllocateTrackingId();
        tracked.x_q8 = detected_points[detected_index].raw_x_q8;
        tracked.y_q8 = detected_points[detected_index].raw_y_q8;
        tracked.pressure = detected_points[detected_index].raw_pressure;
        tracked.raw_x_q8 = detected_points[detected_index].raw_x_q8;
        tracked.raw_y_q8 = detected_points[detected_index].raw_y_q8;
        tracked.raw_pressure = detected_points[detected_index].raw_pressure;
        tracked.active = true;
        tracked.matched = true;
        tracked.just_started = true;
        tracked.dirty = true;
        tracked.missing_frames = 0U;
        tracked.last_seen_ms = now_ms;
      }
    }

    IRAM_ATTR bool TouchPointsChanged()
    {
      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        const TouchPoint& current = touch_points[index];
        if (current.dirty || current.just_ended)
        {
          return true;
        }
      }

      return false;
    }

    void LogTouchPoints()
    {
      if (!TouchPointsChanged())
      {
        return;
      }

      char line[256];
      int written = snprintf(line, sizeof(line), "touch %.0fHz", scan_rate_hz);

      for (uint8_t index = 0U; index < kMaxTouchPoints; ++index)
      {
        const TouchPoint& point = touch_points[index];

        if (point.active)
        {
          const int appended = snprintf(line + written,
                                        sizeof(line) - (size_t)written,
                                        " ID%02u %.2f-%.2f (%.0f)",
                                        (unsigned)point.tracking_id,
                                        DequantizeCoord(point.x_q8) / 3.0f,
                                        DequantizeCoord(point.y_q8) / 3.0f,
                                        (float)point.pressure);
          if ((appended <= 0) || ((size_t)(written + appended) >= sizeof(line)))
          {
            written = (int)sizeof(line) - 1;
            break;
          }
          written += appended;
        }
        else if (point.just_ended)
        {
          const int appended = snprintf(line + written, sizeof(line) - (size_t)written, " ID%02u UP", (unsigned)point.tracking_id);
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
        touch_points[index].dirty = false;
        touch_points[index].just_ended = false;
      }
    }

    void LogRawFrame(const MPECoprocessorLink::MPEData& mpe_data, uint64_t now_us)
    {
      char line[512];

      for (uint8_t y = 0U; y < kGridSize; ++y)
      {
        int written = snprintf(line, sizeof(line), "mpe %llu %.0fHz y%u",
                               (unsigned long long)now_us,
                               scan_rate_hz,
                               (unsigned)y);

        for (uint8_t x = 0U; x < kGridSize; ++x)
        {
          const int appended = snprintf(line + written,
                                        sizeof(line) - (size_t)written,
                                        " %u",
                                        (unsigned)mpe_data[x][y]);
          if ((appended <= 0) || ((size_t)(written + appended) >= sizeof(line)))
          {
            break;
          }
          written += appended;
        }
        ESP_LOGI(kTag, "%s", line);
      }
    }
  }

  void Init()
  {
    coprocessor_link.Init();
  }

  void Start()
  {
    coprocessor_link.Start();
  }

  IRAM_ATTR bool Scan()
  {
    const auto& mpe_data = coprocessor_link.GetMPEData();
    KeyConfig config = mpe_config;
    const uint64_t now_us = esp_timer_get_time();

    if (last_scan_us != 0U)
    {
      const uint64_t delta_us = now_us - last_scan_us;
      if (delta_us > 0U)
      {
        const float instant_scan_rate_hz = 1000000.0f / (float)delta_us;
        if (scan_rate_hz == 0.0f)
        {
          scan_rate_hz = instant_scan_rate_hz;
        }
        else
        {
          scan_rate_hz = scan_rate_hz + (instant_scan_rate_hz - scan_rate_hz) * 0.15f;
        }
      }
    }
    last_scan_us = now_us;

    for (uint8_t y = 0U; y < Y_SIZE; ++y)
    {
      for (uint8_t x = 0U; x < X_SIZE; ++x)
      {
        const uint8_t base_x = (uint8_t)(x * MPECoprocessorLink::SECTOR_GRID_SIZE);
        const uint8_t base_y = (uint8_t)(y * MPECoprocessorLink::SECTOR_GRID_SIZE);
        uint16_t max_reading = 0U;

        for (uint8_t local_y = 0U; local_y < MPECoprocessorLink::SECTOR_GRID_SIZE; ++local_y)
        {
          for (uint8_t local_x = 0U; local_x < MPECoprocessorLink::SECTOR_GRID_SIZE; ++local_x)
          {
            const uint16_t reading = mpe_data[base_x + local_x][base_y + local_y];
            if (reading > max_reading)
            {
              max_reading = reading;
            }
          }
        }

        max_reading = ScaleReading(max_reading);
        pad_force[x][y] = SmoothEMA(max_reading, pad_force[x][y]);

        if (keypad_state[x][y].Update(config, (Fract16)pad_force[x][y]))
        {
          const uint16_t keyID = (1U << 12) + ((uint16_t)x << 6) + y;
          if (NotifyOS(keyID, &keypad_state[x][y]))
          {
            return true;
          }
        }
      }
    }

    // LogRawFrame(mpe_data, now_us);

    return false;
  }
}

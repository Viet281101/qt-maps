#include "FrameStatistics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace qtmaps {
namespace perf {

FrameStatistics::FrameStatistics(int maximumSamples)
    : maximumSamples_(std::max(1, maximumSamples))
{
}

void FrameStatistics::addSample(double framesPerSecond)
{
    if (!std::isfinite(framesPerSecond) || framesPerSecond < 0.0) {
        return;
    }
    if (samples_.size() == maximumSamples_) {
        samples_.removeFirst();
    }
    samples_.append(framesPerSecond);
}

void FrameStatistics::addInteractionFrameTime(double milliseconds)
{
    if (!std::isfinite(milliseconds) || milliseconds <= 0.0) {
        return;
    }
    if (interactionFrameTimesMilliseconds_.size() == maximumSamples_) {
        interactionFrameTimesMilliseconds_.removeFirst();
    }
    interactionFrameTimesMilliseconds_.append(milliseconds);
}

void FrameStatistics::addSettlingFrameTime(double milliseconds)
{
    if (!std::isfinite(milliseconds) || milliseconds <= 0.0) {
        return;
    }
    if (settlingFrameTimesMilliseconds_.size() == maximumSamples_) {
        settlingFrameTimesMilliseconds_.removeFirst();
    }
    settlingFrameTimesMilliseconds_.append(milliseconds);
}

void FrameStatistics::reset()
{
    samples_.clear();
    interactionFrameTimesMilliseconds_.clear();
    settlingFrameTimesMilliseconds_.clear();
}

int FrameStatistics::sampleCount() const
{
    return samples_.size();
}

double FrameStatistics::averageFps() const
{
    if (samples_.isEmpty()) {
        return 0.0;
    }
    return std::accumulate(samples_.cbegin(), samples_.cend(), 0.0) / samples_.size();
}

double FrameStatistics::onePercentLowFps() const
{
    if (samples_.isEmpty()) {
        return 0.0;
    }
    QVector<double> ordered = samples_;
    std::sort(ordered.begin(), ordered.end());
    const int count = std::max(1, static_cast<int>(std::ceil(ordered.size() * 0.01)));
    return std::accumulate(ordered.cbegin(), ordered.cbegin() + count, 0.0) / count;
}

int FrameStatistics::interactionFrameCount() const
{
    return interactionFrameTimesMilliseconds_.size();
}

double FrameStatistics::interactionAverageFps() const
{
    if (interactionFrameTimesMilliseconds_.isEmpty()) {
        return 0.0;
    }
    const double totalMilliseconds = std::accumulate(
        interactionFrameTimesMilliseconds_.cbegin(),
        interactionFrameTimesMilliseconds_.cend(), 0.0);
    return totalMilliseconds > 0.0
        ? interactionFrameTimesMilliseconds_.size() * 1000.0 / totalMilliseconds
        : 0.0;
}

double FrameStatistics::interactionFrameTimePercentile(double percentile) const
{
    if (interactionFrameTimesMilliseconds_.isEmpty() || !std::isfinite(percentile)) {
        return 0.0;
    }
    QVector<double> ordered = interactionFrameTimesMilliseconds_;
    std::sort(ordered.begin(), ordered.end());
    const double boundedPercentile = std::max(0.0, std::min(100.0, percentile));
    const int nearestRank = std::max(
        1, static_cast<int>(std::ceil(boundedPercentile * ordered.size() / 100.0)));
    return ordered.at(std::min(nearestRank - 1, ordered.size() - 1));
}

int FrameStatistics::interactionFrameTimeOverThresholdCount(double thresholdMilliseconds) const
{
    return static_cast<int>(std::count_if(
        interactionFrameTimesMilliseconds_.cbegin(),
        interactionFrameTimesMilliseconds_.cend(),
        [thresholdMilliseconds](double milliseconds) {
            return milliseconds > thresholdMilliseconds;
        }));
}

int FrameStatistics::settlingFrameCount() const
{
    return settlingFrameTimesMilliseconds_.size();
}

double FrameStatistics::settlingFrameAverageMilliseconds() const
{
    if (settlingFrameTimesMilliseconds_.isEmpty()) {
        return 0.0;
    }
    return std::accumulate(settlingFrameTimesMilliseconds_.cbegin(),
                           settlingFrameTimesMilliseconds_.cend(), 0.0)
        / settlingFrameTimesMilliseconds_.size();
}

double FrameStatistics::settlingFrameMaximumMilliseconds() const
{
    if (settlingFrameTimesMilliseconds_.isEmpty()) {
        return 0.0;
    }
    return *std::max_element(settlingFrameTimesMilliseconds_.cbegin(),
                             settlingFrameTimesMilliseconds_.cend());
}

} // namespace perf
} // namespace qtmaps

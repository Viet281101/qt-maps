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

void FrameStatistics::reset()
{
    samples_.clear();
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

} // namespace perf
} // namespace qtmaps

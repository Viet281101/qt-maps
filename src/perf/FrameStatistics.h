#pragma once

#include <QVector>

namespace qtmaps {
namespace perf {

class FrameStatistics
{
public:
    explicit FrameStatistics(int maximumSamples = 3600);

    void addSample(double framesPerSecond);
    void addInteractionFrameTime(double milliseconds);
    void reset();

    int sampleCount() const;
    double averageFps() const;
    double onePercentLowFps() const;
    int interactionFrameCount() const;
    double interactionAverageFps() const;
    double interactionFrameTimePercentile(double percentile) const;

private:
    int maximumSamples_;
    QVector<double> samples_;
    QVector<double> interactionFrameTimesMilliseconds_;
};

} // namespace perf
} // namespace qtmaps

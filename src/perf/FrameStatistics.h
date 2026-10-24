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
    void addSettlingFrameTime(double milliseconds);
    void reset();

    int sampleCount() const;
    double averageFps() const;
    double onePercentLowFps() const;
    int interactionFrameCount() const;
    double interactionAverageFps() const;
    double interactionFrameTimePercentile(double percentile) const;
    int interactionFrameTimeOverThresholdCount(double thresholdMilliseconds) const;
    int settlingFrameCount() const;
    double settlingFrameAverageMilliseconds() const;
    double settlingFrameMaximumMilliseconds() const;

private:
    int maximumSamples_;
    QVector<double> samples_;
    QVector<double> interactionFrameTimesMilliseconds_;
    QVector<double> settlingFrameTimesMilliseconds_;
};

} // namespace perf
} // namespace qtmaps

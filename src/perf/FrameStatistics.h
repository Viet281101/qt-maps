#pragma once

#include <QVector>

namespace qtmaps {
namespace perf {

class FrameStatistics
{
public:
    explicit FrameStatistics(int maximumSamples = 3600);

    void addSample(double framesPerSecond);
    void reset();

    int sampleCount() const;
    double averageFps() const;
    double onePercentLowFps() const;

private:
    int maximumSamples_;
    QVector<double> samples_;
};

} // namespace perf
} // namespace qtmaps

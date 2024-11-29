#include "CustomChartUtils.h"

#include <QtCharts>

qreal ChartUtils::minX = std::numeric_limits<qreal>::max();;
qreal ChartUtils::maxX = std::numeric_limits<qreal>::lowest();
qreal ChartUtils::minY = std::numeric_limits<qreal>::max();
qreal ChartUtils::maxY = std::numeric_limits<qreal>::lowest();

void ChartUtils::updateXLimits(const QChart *chart) {
    // Iteration to find the min/max values
    for (const auto &series: chart->series()) {
        if (const auto lineSeries = qobject_cast<QLineSeries *>(series)) {
            for (const QPointF &point: lineSeries->points()) {
                minX = qMin(minX, point.x());
                maxX = qMax(maxX, point.x());
                minY = qMin(minY, point.y());
                maxY = qMax(maxY, point.y());
            }
        }
    }
    const qreal factor = 0.1 * maxY;
    maxY += factor;
    minY -= factor;
}

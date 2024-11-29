#ifndef CUSTOMCHARTUTILS_H
#define CUSTOMCHARTUTILS_H
#pragma once

#include <QtCharts/QLineSeries>
#include <QtTypes>

class ChartUtils {
public:
    static qreal minX;
    static qreal maxX;
    static qreal minY;
    static qreal maxY;

    static void updateXLimits(const QChart *chart);
};

#endif // CUSTOMCHARTUTILS_H

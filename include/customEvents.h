#ifndef CUSTOMEVENTS_H
#define CUSTOMEVENTS_H
#pragma once

/*
All-in-one custom zoom and tracking capabilities for Qt charts series

Copyright (C) 2025 Criogenox

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtTypes>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QTimer>
#include <QToolTip>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QXYSeries>

class ZoomAndScroll final : public QChartView {
    Q_OBJECT

public:
    explicit ZoomAndScroll(QChart *chart, QWidget *parent = nullptr);

signals:
    void mouseMoved(QPointF mousePos,
                    QMouseEvent *event,
                    QVector<qreal> &limits);

    void hideWhenMove();

private:
    bool resizeHorZoom;
    bool resizeVerZoom;
    QPoint lastMousePos;
    QPoint rubberBandStartPos;
    QScopedPointer<QGraphicsRectItem> rubberBandItem;
    qreal xMin{}, xMax{}, yMin{}, yMax{};
    QVector<qreal> limits{};

    struct SeriesIntersection {
        QXYSeries *series{};
        QPointF point;
    };

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void clearRubberBand();

    void resetChartToOriginal() const;

    void rangeUpdate();

public:
    bool toggleState;
    bool toggleFocus;
    bool toggleLines;
    qreal minX{};
    qreal maxX{};
    qreal minY{};
    qreal maxY{};

    void updateXLimits(const QChart *chart);

    void updateIntersections(QXYSeries *series, const QPointF &point);

    [[nodiscard]] QXYSeries *findBottomSeries() const;

    QList<SeriesIntersection> currentIntersections;
};

class LineSeries;
class ScatterSeries;
class SplineSeries;

template<typename SeriesType>
class Methods {
public:
    explicit Methods(SeriesType *ptr, ZoomAndScroll *m_chartView);

    virtual ~Methods();

protected:
    struct TooltipData {
        QString text;
        QPoint position;
        QColor color;
    };

    struct Intercerp {
        qreal distance{};
        QPointF pos;
    };

    QTimer *tooltipTimer{};
    QGraphicsEllipseItem *bullet;
    QList<QLabel *> toolTips;
    QList<QGraphicsLineItem *> lines;
    QList<QGraphicsDropShadowEffect *> shadowEffect;
    SeriesType *ptr;
    ZoomAndScroll *m_chartView;

    static qreal distanceToLineSegment(const QPointF &point,
                                       const QPointF &lineStart,
                                       const QPointF &lineEnd);

    virtual QList<Intercerp> findIntersection(const QPointF &mouse);

    void handleTooltipOnFocus(const QPointF &chartPos, const QMouseEvent *event);

    void handleTooltipForTracking(
        const QPointF &chartPos, const QPointF &mousePos, QVector<qreal> &limits);

    void createLines(int n);

    void updateVerticalLine(
        const QPointF &mousePos, const QPointF &IPpixel, QVector<qreal> &limits);

    void setTooltips(const QPointF &intersectionPoint, const QPointF &IPpixel);

    void createTooltips(const QList<TooltipData> &tooltipDataList);

    void drawBullet(const QPointF &point);

    void deleteTooltip();

    void hideTooltip();
};

class LineSeries final : public QLineSeries, public Methods<LineSeries> {
    Q_OBJECT

public:
    explicit LineSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

public slots:
    void hideAll();

private slots:
    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);

private:
    ZoomAndScroll *m_chartView;
};

class ScatterSeries final : public QScatterSeries, public Methods<ScatterSeries> {
    Q_OBJECT

public:
    explicit ScatterSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

public slots:
    void hideAll();

private slots:
    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);

private:
    ZoomAndScroll *m_chartView;
};

class SplineSeries final : public QSplineSeries, public Methods<SplineSeries> {
    Q_OBJECT

public:
    explicit SplineSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

public slots:
    void hideAll();

private slots:
    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);

private:
    ZoomAndScroll *m_chartView;

    QList<Intercerp> findIntersection(const QPointF &mouse) override;

    static constexpr qreal T = 1; // Catmull-Rom tension parameter

    static QPointF catmullRomInterpolate(const qreal t,
                                         const QPointF &p0,
                                         const QPointF &p1,
                                         const QPointF &p2,
                                         const QPointF &p3) {
        const qreal t2 = t * t;
        const qreal t3 = t2 * t;

        const qreal h1 = -T * t3 + 2 * T * t2 - T * t;
        const qreal h2 = (2 - T) * t3 + (T - 3) * t2 + 1;
        const qreal h3 = (T - 2) * t3 + (3 - 2 * T) * t2 + T * t;
        const qreal h4 = T * t3 - T * t2;

        return {
            h1 * p0.x() + h2 * p1.x() + h3 * p2.x() + h4 * p3.x(),
            h1 * p0.y() + h2 * p1.y() + h3 * p2.y() + h4 * p3.y()
        };
    }
};
#endif

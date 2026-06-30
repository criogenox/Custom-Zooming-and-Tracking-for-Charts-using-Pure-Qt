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

#include <chrono>
#include <functional>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QTimer>
#include <QToolTip>
#include <QtCharts/QChartView>
#include <QXYSeries>
#include <QFutureWatcher>

struct TrackResult {
    qreal distance{};
    QPointF pos;
    QPointF IPpixel;
    bool isValid = false;
};

class ZoomAndScroll final : public QChartView {
    Q_OBJECT

public:
    explicit ZoomAndScroll(QChart *chart, QWidget *parent = nullptr);

    // Batched, single-task tracking pipeline.
    // Worker-thread compute callback.
    // GUI-thread render callback.
    using TrackPrepareFn = std::function<void()>;
    using TrackComputeFn = std::function<TrackResult(const QPointF &, const QPointF &,
                                                     const QVector<qreal> &, bool)>;
    using TrackRenderFn = std::function<void(const TrackResult &)>;

    void registerTracker(TrackPrepareFn prepare, TrackComputeFn compute, TrackRenderFn render);

signals:
    void mouseMoved(QPointF mousePos,
                    QMouseEvent *event,
                    const QVector<qreal> &limits);

    void hideWhenMove();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool resizeHorZoom;
    bool resizeVerZoom;
    QPoint lastMousePos;
    QPoint rubberBandStartPos;
    std::chrono::steady_clock::time_point lastEventTime{std::chrono::steady_clock::now()};
    QScopedPointer<QGraphicsRectItem> rubberBandItem;
    QVector<qreal> limits{};

    struct SeriesIntersection {
        QXYSeries *series{};
        QPointF point;
    };

    void clearRubberBand();

    void resetChartToOriginal() const;

public:
    void rangeUpdate();

    bool toggleState;
    bool toggleFocus;
    bool toggleLines;
    qreal minX{};
    qreal maxX{};
    qreal minY{};
    qreal maxY{};
    qreal xMin{};
    qreal xMax{};
    qreal yMin{};
    qreal yMax{};

    void updateXLimits(const QChart *chart);

    void updateIntersections(QXYSeries *series, const QPointF &point);

    [[nodiscard]] QXYSeries *findBottomSeries() const;

    QList<SeriesIntersection> currentIntersections;

private:
    QList<TrackPrepareFn> m_prepareFns;
    QList<TrackComputeFn> m_computeFns;
    QList<TrackRenderFn> m_renderFns;
    QFutureWatcher<QList<TrackResult> > *m_batchWatcher{};

    void runBatchTracking(const QPointF &chartPos, const QPointF &mousePos,
                          const QVector<qreal> &limits, bool focusEnabled);

    void onBatchFinished();
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
        QPointF IPpixel;
        bool isValid = false;
    };

    int m_tooltipTimeout = 1000;

    QList<QPointF> m_points;
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

    virtual Intercerp findIntersection(const QList<QPointF> &points, bool focusEnabled,
                                       const QPointF &chartPos, const QPointF &mousePos,
                                       const QVector<qreal> &limits);

    void renderTracking(const TrackResult &result);

    void registerBatchTracking();

    void handleTooltipOnFocus(const QPointF &chartPos, const QMouseEvent *event);

    void createLines(int n);

    void updateVerticalLine(
        const QPointF &mousePos, const QPointF &IPpixel, const QVector<qreal> &limits);

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
                      const QVector<qreal> &limits);

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
                      const QVector<qreal> &limits);

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
                      const QVector<qreal> &limits);

protected:
    Intercerp findIntersection(const QList<QPointF> &points, bool focusEnabled,
                               const QPointF &chartPos, const QPointF &mousePos,
                               const QVector<qreal> &limits) override;

private:
    ZoomAndScroll *m_chartView;

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

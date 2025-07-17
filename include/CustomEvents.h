#ifndef CUSTOMEVENTS_H
#define CUSTOMEVENTS_H
#pragma once

#include <QtTypes>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QTimer>
#include <QToolTip>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

class ZoomAndScroll final : public QChartView {
    Q_OBJECT

public:
    explicit ZoomAndScroll(QChart *chart, QWidget *parent = nullptr);

    ~ZoomAndScroll() override = default;

    bool toggleState;
    bool toggleFocus;
    bool toggleLines;
    qreal minX;
    qreal maxX;
    qreal minY;
    qreal maxY;

    void updateXLimits(const QChart *chart);

signals:
    void mouseMoved(QPointF mousePos,
                    QMouseEvent *event,
                    QVector<qreal> &limits);

    void hide_when_move();

private:
    bool resizeZoom;
    QPoint lastMousePos;
    QPoint rubberBandStartPos;
    QScopedPointer<QGraphicsRectItem> rubberBandItem;
    qreal xMin{}, xMax{}, yMin{}, yMax{};
    QVector<qreal> limits{};

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void clearRubberBand();

    void resetChartToOriginal() const;

    void rangeUpdate();
};

class LineSeries;
class ScatterSeries;
class SplineSeries;

template<typename SeriesType>
class Methods {
public:
    explicit Methods(SeriesType *ptr, ZoomAndScroll *m_chartView);

    ~Methods();

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

    [[nodiscard]] QList<Intercerp> findIntersection(QPointF mouse) const;

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

    ZoomAndScroll *m_chartView;

public slots:
    void hideAll();

    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);
};

class ScatterSeries final : public QScatterSeries, public Methods<ScatterSeries> {
    Q_OBJECT

public:
    explicit ScatterSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

    ZoomAndScroll *m_chartView;

public slots:
    void hideAll();

    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);
};

class SplineSeries final : public QSplineSeries, public Methods<SplineSeries> {
    Q_OBJECT

public:
    explicit SplineSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

    ZoomAndScroll *m_chartView;

public slots:
    void hideAll();

    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);
};
#endif

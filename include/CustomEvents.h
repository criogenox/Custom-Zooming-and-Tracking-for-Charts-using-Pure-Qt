#ifndef CUSTOMEVENTS_H
#define CUSTOMEVENTS_H

#pragma once

#include <QGraphicsEllipseItem>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QSplineSeries>
#include <QTimer>
#include <QToolTip>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class ZoomAndScroll final : public QChartView {
    Q_OBJECT

public:
    explicit ZoomAndScroll(QChart *chart, QWidget *parent = nullptr);

    ~ZoomAndScroll() override = default;

    bool toggleState;
    bool toggleFocus;
    bool toggleLines;

signals:
    void mouseMoved(QPointF mousePos,
                    QMouseEvent *event,
                    QVector<qreal> &limits);

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool resizeZoom;
    QPoint lastMousePos;
    QPoint rubberBandStartPos;
    QScopedPointer<QGraphicsRectItem> rubberBandItem;
    qreal xMin{}, xMax{}, yMin{}, yMax{};
    QVector<qreal> limits{};

    void clearRubberBand();

    void resetChartToOriginal() const;

    void rangeUpdate();
};

class TrackingSeries final : public QSplineSeries {
    Q_OBJECT

public:
    explicit TrackingSeries(ZoomAndScroll *chartView, QObject *parent = nullptr);

    ~TrackingSeries() override;

    struct TooltipData {
        QString text;
        QPoint position;
        QColor color;
    };

    struct Intercerp {
        qreal distance{};
        QPointF pos;
    };

private slots:
    void hideTooltip();

    void onMouseMoved(QPointF mousePos,
                      const QMouseEvent *event,
                      QVector<qreal> &limits);

private:
    ZoomAndScroll *m_chartView;
    QList<QGraphicsLineItem *> lines;
    QList<QLabel *> toolTips;
    QGraphicsEllipseItem *bullet;
    QTimer *tooltipTimer;
    QList<QGraphicsDropShadowEffect *> shadowEffect;
    static qreal distanceToLineSegment(const QPointF &point,
                                       const QPointF &lineStart,
                                       const QPointF &lineEnd);

    QList<Intercerp> findIntersection(QPointF mouse);

    void handleTooltipOnFocus(const QPointF &chartPos, const QMouseEvent *event);

    void handleTooltipForTracking(
        const QPointF &chartPos, const QPointF &mousePos, QVector<qreal> &limits);

    void createLines(int n);

    void updateVerticalLine(
        const QPointF &mousePos, const QPointF &IPpixel, QVector<qreal> &limits);

    void setTooltips(const QPointF &intersectionPoint, const QPointF &IPpixel);

    void createTooltips(const QList<TooltipData> &tooltipDataList);

    void drawBullet(const QPointF &point);

    void hideAll();

    void deleteTooltip();
};

#endif

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

#include "customEvents.h"
#include <QtCharts/QValueAxis>
#include <QScopedPointer>
#include <QtConcurrent/QtConcurrent>

ZoomAndScroll::ZoomAndScroll(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
      , resizeHorZoom(false)
      , resizeVerZoom(false)
      , rubberBandItem(nullptr)
      , toggleState(false)
      , toggleFocus(false)
      , toggleLines(false) {
    setMouseTracking(true); // Enable mouse tracking (runs once)
    // Single watcher for the batched, all-series intersection task.
    m_batchWatcher = new QFutureWatcher<QList<TrackResult> >(this);
    connect(m_batchWatcher, &QFutureWatcher<QList<TrackResult> >::finished,
            this, [this]() { onBatchFinished(); });
}

void ZoomAndScroll::registerTracker(TrackPrepareFn prepare, TrackComputeFn compute,
                                    TrackRenderFn render) {
    m_prepareFns.append(std::move(prepare));
    m_computeFns.append(std::move(compute));
    m_renderFns.append(std::move(render));
}

void ZoomAndScroll::runBatchTracking(const QPointF &chartPos, const QPointF &mousePos,
                                     const QVector<qreal> &lims, const bool focusEnabled) {
    if (m_computeFns.isEmpty())
        return;

    // Cancel the in-flight batch (if any) before dispatching a new one.
    if (m_batchWatcher->isRunning()) {
        m_batchWatcher->cancel();
    }

    // Refresh every series' snapshot on the GUI thread before dispatching.
    // Worker only ever reads already-detached, immutable point data.
    for (const auto &prepare: m_prepareFns) {
        prepare();
    }

    // Copy the compute callbacks.
    // Worker only reads its series' immutable cached snapshot (allocation-free).
    const QList<TrackComputeFn> computeFns = m_computeFns;
    const auto future = QtConcurrent::run(
        [computeFns, chartPos, mousePos, lims, focusEnabled]() {
            QList<TrackResult> results;
            results.reserve(computeFns.size());
            for (const auto &compute: computeFns) {
                results.append(compute(chartPos, mousePos, lims, focusEnabled));
            }
            return results;
        });
    m_batchWatcher->setFuture(future);
}

void ZoomAndScroll::onBatchFinished() {
    if (m_batchWatcher->isCanceled())
        return;

    const QList<TrackResult> results = m_batchWatcher->result();
    // Render every series' result together so currentIntersections &
    // crosshair/labels are updated as one atomic frame.
    const int n = std::min<int>(static_cast<int>(results.size()), static_cast<int>(m_renderFns.size()));
    for (int i = 0; i < n; ++i) {
        m_renderFns[i](results[i]);
    }
}

void ZoomAndScroll::updateXLimits(const QChart *chart) {
    if (!chart->series().isEmpty()) {
        qreal x_Min = std::numeric_limits<qreal>::infinity();
        qreal x_Max = -std::numeric_limits<qreal>::infinity();
        qreal y_Min = std::numeric_limits<qreal>::infinity();
        qreal y_Max = -std::numeric_limits<qreal>::infinity();
        bool hasData = false;

        for (QAbstractSeries *s: chart->series()) {
            const auto *xy = qobject_cast<QXYSeries *>(s);
            if (!xy || !xy->isVisible())
                continue;

            const auto pts = xy->points();
            if (pts.isEmpty())
                continue;

            for (const QPointF &p: pts) {
                if (!std::isfinite(p.x()) || !std::isfinite(p.y()))
                    continue;
                hasData = true;
                x_Min = std::min(x_Min, p.x());
                x_Max = std::max(x_Max, p.x());
                y_Min = std::min(y_Min, p.y());
                y_Max = std::max(y_Max, p.y());
            }
        }

        if (!hasData) {
            minX = maxX = 0;
            minY = maxY = 0;
            return;
        }

        auto expandIfFlat = [](qreal &a, qreal &b) {
            if (qFuzzyCompare(a, b)) {
                const qreal eps = std::max<qreal>(1e-6, std::abs(a) * 0.05);
                a -= eps;
                b += eps;
            }
        };
        expandIfFlat(x_Min, x_Max);
        expandIfFlat(y_Min, y_Max);

        const qreal yPad = (y_Max - y_Min) * 0.10; // 10% padding of the y range
        x_Min -= (x_Max - x_Min) * 0.05; // Added 5% padding to x range as well for better visualization
        x_Max += (x_Max - x_Min) * 0.05;
        y_Min -= yPad;
        y_Max += yPad;

        minX = x_Min;
        maxX = x_Max;
        minY = y_Min;
        maxY = y_Max;
    }
}

void ZoomAndScroll::rangeUpdate() {
    // Get the X axis
    for (QAbstractAxis *axis: chart()->axes(Qt::Horizontal)) {
        if (const auto *xAxis = qobject_cast<QValueAxis *>(axis)) {
            xMin = xAxis->min();
            xMax = xAxis->max();
            break;
        }
    }

    // Get the Y axis
    for (QAbstractAxis *axis: chart()->axes((Qt::Vertical))) {
        if (const auto *yAxis = qobject_cast<QValueAxis *>(axis)) {
            yMin = yAxis->min();
            yMax = yAxis->max();
            break;
        }
    }
}

void ZoomAndScroll::resetChartToOriginal() const {
    const auto hAxes = chart()->axes(Qt::Horizontal);
    const auto vAxes = chart()->axes(Qt::Vertical);
    auto *xAxis = hAxes.isEmpty() ? nullptr : dynamic_cast<QValueAxis *>(hAxes.first());
    auto *yAxis = vAxes.isEmpty() ? nullptr : dynamic_cast<QValueAxis *>(vAxes.first());

    if (xAxis) {
        xAxis->setRange(minX, maxX);
    }
    if (yAxis) {
        yAxis->setRange(minY, maxY);
    }
    chart()->update(); // Chart view refreshing
}

void ZoomAndScroll::keyPressEvent(QKeyEvent *event) {
    // Check if keys are pressed
    if (event->key() == Qt::Key_T) {
        toggleState = !toggleState; // Track-line labeling
    }
    if (event->key() == Qt::Key_U) {
        toggleLines = !toggleLines; // Crosshair | truncated track-lines
    }
    if (event->key() == Qt::Key_Y) {
        if (QToolTip::isVisible()) {
            QToolTip::hideText(); // Hide tooltip immediately when is disabled
        }
        // Toggling the cursor-hover stops/starts the batched track-line
        // labeling. The already-shown tracking lines/labels/bullets would
        // be hidden instantly whenever this mode is switched.
        if (toggleState) {
            emit hideWhenMove();
        }
        toggleFocus = !toggleFocus; // Tooltips on cursor hover
    }
    if (event->key() == Qt::Key_R) {
        resizeHorZoom = !resizeHorZoom; // Horizontal panning (x axis clipping)
        if (resizeVerZoom) {
            resizeVerZoom = !resizeVerZoom; // One axis clipping at a time
        }
    }
    if (event->key() == Qt::Key_E) {
        resizeVerZoom = !resizeVerZoom; // Vertical panning (y axis clipping)
        if (resizeHorZoom) {
            resizeHorZoom = !resizeHorZoom; // One axis clipping at a time
        }
    }
}

void ZoomAndScroll::mousePressEvent(QMouseEvent *event) {
    if (chart() && !chart()->series().isEmpty()) {
        // Zoom-in by drag/selection rubber band area
        if (event->button() == Qt::LeftButton) {
            event->accept();
            rubberBandStartPos = event->pos();
            // rubberBandItem as QScopedPointer
            rubberBandItem.reset(new QGraphicsRectItem());
            rubberBandItem->setPen(QPen(Qt::DashLine));
            rubberBandItem->setBrush(QBrush(QColor(255, 255, 102, 50))); // Semi-transparent red
            chart()->scene()->addItem(rubberBandItem.data());
        } else if (event->button() == Qt::RightButton) {
            // Change to panning by drag
            event->accept();
            lastMousePos = event->pos();
            setDragMode(ScrollHandDrag);
            setCursor(Qt::ClosedHandCursor);
        }
        rangeUpdate();
    }
}

void ZoomAndScroll::clearRubberBand() {
    if (rubberBandItem) {
        rubberBandItem.reset();
    }
}

void ZoomAndScroll::mouseReleaseEvent(QMouseEvent *event) {
    if (chart() && !chart()->series().isEmpty()) {
        if (rubberBandItem) {
            QRectF rubberBandRect(rubberBandStartPos, event->pos());
            rubberBandRect = rubberBandRect.normalized();
            QRectF plotArea = chart()->plotArea();

            if (rubberBandRect.intersects(plotArea)) {
                chart()->zoomIn(rubberBandRect);
            }

            clearRubberBand();
        }

        if (event->button() == Qt::RightButton) {
            setDragMode(NoDrag);
            setCursor(Qt::ArrowCursor); // Reset cursor to default
        }
        rangeUpdate();
    }
}

void ZoomAndScroll::mouseDoubleClickEvent(QMouseEvent *event) {
    if (chart() && !chart()->series().isEmpty()) {
        event->accept();
        // Double-click to fit the chart with original axes ranges.
        if (event->button() == Qt::LeftButton) {
            // -----------------
            if (toggleState) {
                emit hideWhenMove();
            }
            // -----------------
            resetChartToOriginal();
        }
        rangeUpdate();
    }
}

void ZoomAndScroll::wheelEvent(QWheelEvent *event) {
    if (chart() && !chart()->series().isEmpty()) {
        if (toggleState) {
            emit hideWhenMove();
        }
        // -----------------
        const QRectF plotArea = chart()->plotArea();

        // No zoom action if wheel scrolling is made outside the plot area
        if (event->position().x() < plotArea.x() || event->position().y() < plotArea.y() ||
            event->position().x() > plotArea.x() + plotArea.width() ||
            event->position().y() > plotArea.y() + plotArea.height())
            return;

        const QPointF mousePos = event->position() - plotArea.topLeft();
        const qreal numDegrees = static_cast<qreal>(event->angleDelta().y()) / 8;
        const qreal factor = numDegrees > 0 ? 1.2 : 0.8; // Zoom in or out

        const auto allAxes = chart()->axes();
        auto *xAxis = !allAxes.empty() ? qobject_cast<QValueAxis *>(allAxes[0]) : nullptr;
        auto *yAxis = allAxes.size() > 1 ? qobject_cast<QValueAxis *>(allAxes[1]) : nullptr;

        if (xAxis && yAxis) {
            // Get current axes limits
            const qreal oldMinX = xAxis->min();
            const qreal oldMaxX = xAxis->max();
            const qreal oldMinY = yAxis->min();
            const qreal oldMaxY = yAxis->max();

            const qreal rangeX = oldMaxX - oldMinX;
            const qreal rangeY = oldMaxY - oldMinY;

            // New axes limits based on mouse position and zoom factor
            qreal newMinX = mousePos.x() / plotArea.width() * rangeX + oldMinX -
                            (rangeX / factor) * (mousePos.x() / plotArea.width());
            qreal newMaxX = mousePos.x() / plotArea.width() * rangeX + oldMinX +
                            (rangeX / factor) * (1 - mousePos.x() / plotArea.width());
            const qreal invertedY = plotArea.height() - mousePos.y(); // Invert Y-coordinate
            qreal newMinY = invertedY / plotArea.height() * rangeY + oldMinY -
                            (rangeY / factor) * (invertedY / plotArea.height());
            qreal newMaxY = invertedY / plotArea.height() * rangeY + oldMinY +
                            (rangeY / factor) * (1 - invertedY / plotArea.height());

            // Clamp new limits to min/max values
            newMinX = std::max(newMinX, minX);
            newMaxX = std::min(newMaxX, maxX);
            newMinY = std::max(newMinY, minY);
            newMaxY = std::min(newMaxY, maxY);

            // Handle horizontal panning
            if (resizeHorZoom) {
                if (numDegrees > 0) {
                    newMinX = qMax(xMin, newMinX);
                    newMaxX = qMin(xMax, newMaxX);
                } else {
                    newMinX = qMin(xMin, newMinX);
                    newMaxX = qMax(xMax, newMaxX);
                }
                xAxis->setRange(newMinX, newMaxX);
            } else if (resizeVerZoom) {
                if (numDegrees > 0) {
                    newMinY = qMax(yMin, newMinY);
                    newMaxY = qMin(yMax, newMaxY);
                } else {
                    newMinY = qMin(yMin, newMinY);
                    newMaxY = qMax(yMax, newMaxY);
                }
                yAxis->setRange(newMinY, newMaxY);
            } else {
                if (numDegrees > 0) {
                    newMinX = qMax(xMin, newMinX);
                    newMaxX = qMin(xMax, newMaxX);
                    newMinY = qMax(yMin, newMinY);
                    newMaxY = qMin(yMax, newMaxY);
                } else {
                    newMinX = qMin(xMin, newMinX);
                    newMaxX = qMax(xMax, newMaxX);
                    newMinY = qMin(yMin, newMinY);
                    newMaxY = qMax(yMax, newMaxY);
                }
                xAxis->setRange(newMinX, newMaxX);
                yAxis->setRange(newMinY, newMaxY);
            }
        }
        event->accept();
        rangeUpdate();
    }
}

void ZoomAndScroll::mouseMoveEvent(QMouseEvent *event) {
    if (chart() && !chart()->series().isEmpty()) {
        const auto currentTime = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>
            (currentTime - lastEventTime).count() < 16) {
            event->accept();
            return;
        }
        lastEventTime = currentTime;

        // Axis ranges only change on zoom/scroll/reset, so the cached
        // xMin/xMax/yMin/yMax are refreshed there (and after pan-scroll below)
        // instead of rescanning the axes on every mouse-move frame.
        if (rubberBandItem) {
            // -----------------
            if (toggleState) {
                emit hideWhenMove();
            }
            // -----------------
            // Set the area to zoom
            QRectF rubberBandRect(rubberBandStartPos, event->pos());
            rubberBandRect = rubberBandRect.normalized();
            rubberBandItem->setRect(rubberBandRect);
        }

        if (event->buttons() & Qt::RightButton) {
            qreal deltaX = -(event->pos().x() - lastMousePos.x());
            qreal deltaY = (event->pos().y() - lastMousePos.y());
            constexpr qreal sensitivity = 0.8; // Panning sensitivity factor
            deltaX *= sensitivity;
            deltaY *= sensitivity;

            if (std::signbit(deltaX)) {
                if (xMin <= minX) {
                    deltaX = 0.00;
                }
            }
            if (!std::signbit(deltaX)) {
                if (xMax >= maxX) {
                    deltaX = 0.00;
                }
            }

            if (std::signbit(deltaY)) {
                if (yMin <= minY) {
                    deltaY = 0.00;
                }
            }
            if (!std::signbit(deltaY)) {
                if (yMax >= maxY) {
                    deltaY = 0.00;
                }
            }
            chart()->scroll(deltaX, deltaY);
            event->accept();
            // Panning changes the axis ranges, so refresh the cached values here.
            rangeUpdate();
        }

        lastMousePos = event->pos();

        const QPointF mousePos = mapToScene(event->pos());
        // Mouse position as emitted signal
        const QVector<qreal> viewLimits{xMin, xMax, yMin, yMax};
        if (!rubberBandItem) {
            // The signal drives the synchronous, GUI-thread focus-labeling &
            // hide the logic in each series.
            emit mouseMoved(mousePos, event, viewLimits);

            // Line-intersection tracking is computed for all series in one
            // shared background task.
            if (toggleState && !toggleFocus) {
                const QPointF chartPos = chart()->mapToValue(mousePos);
                const bool isVisible = chartPos.x() >= viewLimits[0] && chartPos.x() <= viewLimits[1]
                                       && chartPos.y() >= viewLimits[2] && chartPos.y() <= viewLimits[3];
                if (isVisible) {
                    runBatchTracking(chartPos, mousePos, viewLimits, toggleFocus);
                }
            }
        }
    }
}

void ZoomAndScroll::updateIntersections(QXYSeries *series, const QPointF &point) {
    // Removal old intersection point for each series, if it exists
    for (auto it = currentIntersections.begin(); it != currentIntersections.end();) {
        if (it->series == series) {
            it = currentIntersections.erase(it);
        } else {
            ++it;
        }
    }

    // Adding a new intersection point
    currentIntersections.append({series, point});
}

QXYSeries *ZoomAndScroll::findBottomSeries() const {
    if (currentIntersections.isEmpty()) {
        return nullptr;
    }

    // Find series with the lowest Y-axis value
    QXYSeries *bottomSeries = currentIntersections.first().series;
    qreal lowestY = currentIntersections.first().point.y();

    for (const auto &[series, point]: currentIntersections) {
        if (point.y() < lowestY) {
            lowestY = point.y();
            bottomSeries = series;
        }
    }
    return bottomSeries;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

LineSeries::LineSeries(ZoomAndScroll *chartView, QObject *parent)
    : QLineSeries(parent), Methods(this, chartView)
      , m_chartView(chartView) {
    // Signal/slot to transfer mouse move events to tracking method
    connect(m_chartView,
            &ZoomAndScroll::mouseMoved,
            this,
            &LineSeries::onMouseMoved,
            Qt::DirectConnection);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hideWhenMove,
            this,
            &LineSeries::hideAll,
            Qt::DirectConnection);
    // Signal/slot to manage the timed hiding of lines/labels/bullets
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer,
            &QTimer::timeout,
            this,
            &LineSeries::hideAll,
            Qt::DirectConnection);
}

void LineSeries::hideAll() {
    if (!m_chartView->currentIntersections.isEmpty()) {
        m_chartView->currentIntersections.clear();
    }
    if (!lines.isEmpty()) {
        for (const auto line: lines) {
            line->hide();
        }
        for (const auto tip: toolTips) {
            tip->hide();
        }
    }
    if (bullet) {
        bullet->hide();
    }
}

void LineSeries::onMouseMoved(const QPointF mousePos,
                              const QMouseEvent *event,
                              const QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    } else if (!(m_chartView->toggleState && isVisible)) {
        // Line-intersection tracking is now driven by the shared batched task in
        // ZoomAndScroll; only hide it here when neither focus-labeling nor tracking
        // is active for this frame.
        hideAll();
    }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ScatterSeries::ScatterSeries(ZoomAndScroll *chartView, QObject *parent)
    : QScatterSeries(parent), Methods(this, chartView)
      , m_chartView(chartView) {
    // Signal/slot to transfer mouse move events to tracking method
    connect(m_chartView,
            &ZoomAndScroll::mouseMoved,
            this,
            &ScatterSeries::onMouseMoved,
            Qt::DirectConnection);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hideWhenMove,
            this,
            &ScatterSeries::hideAll,
            Qt::DirectConnection);
    // Signal/slot to manage the timed hiding of labels
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer,
            &QTimer::timeout,
            this,
            &ScatterSeries::hideAll,
            Qt::DirectConnection);
}

void ScatterSeries::hideAll() {
    if (!m_chartView->currentIntersections.isEmpty()) {
        m_chartView->currentIntersections.clear();
    }
    if (!lines.isEmpty()) {
        for (const auto line: lines) {
            line->hide();
        }
        for (const auto tip: toolTips) {
            tip->hide();
        }
    }
    if (bullet) {
        bullet->hide();
    }
}

void ScatterSeries::onMouseMoved(const QPointF mousePos,
                                 const QMouseEvent *event,
                                 const QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    } else if (!(m_chartView->toggleState && isVisible)) {
        // Line-intersection tracking is now driven by the shared batched task in
        // ZoomAndScroll; only hide it here when neither focus-labeling nor tracking
        // is active for this frame.
        hideAll();
    }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SplineSeries::SplineSeries(ZoomAndScroll *chartView, QObject *parent)
    : QSplineSeries(parent), Methods(this, chartView)
      , m_chartView(chartView) {
    // Signal/slot to transfer mouse move events to tracking method
    connect(m_chartView,
            &ZoomAndScroll::mouseMoved,
            this,
            &SplineSeries::onMouseMoved,
            Qt::DirectConnection);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hideWhenMove,
            this,
            &SplineSeries::hideAll,
            Qt::DirectConnection);
    // Signal/slot to manage the timed hiding of labels
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer,
            &QTimer::timeout,
            this, &SplineSeries::hideAll,
            Qt::DirectConnection);
}

void SplineSeries::hideAll() {
    if (!m_chartView->currentIntersections.isEmpty()) {
        m_chartView->currentIntersections.clear();
    }
    if (!lines.isEmpty()) {
        for (const auto line: lines) {
            line->hide();
        }
        if (!toolTips.isEmpty()) {
            for (const auto tip: toolTips) {
                tip->hide();
            }
        }
    }
    if (bullet) {
        bullet->hide();
    }
}

void SplineSeries::onMouseMoved(const QPointF mousePos,
                                const QMouseEvent *event,
                                const QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    } else if (!(m_chartView->toggleState && isVisible)) {
        // Line-intersection tracking is now driven by the shared batched task in
        // ZoomAndScroll; only hide it here when neither focus-labeling nor tracking
        // is active for this frame.
        hideAll();
    }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename SeriesType>
Methods<SeriesType>::Methods(SeriesType *ptr, ZoomAndScroll *m_chartView)
    : bullet(nullptr),
      ptr(ptr),
      m_chartView(m_chartView) {
    registerBatchTracking();
}

// Single batched task
template<typename SeriesType>
void Methods<SeriesType>::registerBatchTracking() {
    m_chartView->registerTracker(
        // prepare (GUI thread): refresh the immutable snapshot only when needed.
        [this]() {
            if (m_points.size() != ptr->count()) {
                m_points = ptr->points();
            }
        },
        // compute (worker thread): pure, reads only the cached snapshot.
        [this](const QPointF &chartPos, const QPointF &mousePos,
               const QVector<qreal> &limits, const bool focusEnabled) -> TrackResult {
            const Intercerp r = findIntersection(m_points, focusEnabled, chartPos, mousePos, limits);
            return TrackResult{r.distance, r.pos, r.IPpixel, r.isValid};
        },
        // render (GUI thread): draw lines/labels/bullet for this series.
        [this](const TrackResult &result) {
            renderTracking(result);
        });
}

template<typename SeriesType>
void Methods<SeriesType>::deleteTooltip() {
    if (!toolTips.isEmpty()) {
        if (!shadowEffect.isEmpty()) {
            qDeleteAll(shadowEffect);
            shadowEffect.clear();
        }
        qDeleteAll(toolTips);
        toolTips.clear();
    }
}

template<typename SeriesType>
Methods<SeriesType>::~Methods() {
    ptr->hideTooltip(); // Hide before destruction
    deleteTooltip(); // Correct deallocation memory order (no double deallocation)
    qDeleteAll(lines); // Track-lines
    lines.clear();
    if (bullet) {
        delete bullet; // Intersection-point bullet
        bullet = nullptr; // Avoiding dangling pointer
    }
}

// Labeling by mouse hovering
template<typename SeriesType>
void Methods<SeriesType>::handleTooltipOnFocus(const QPointF &chartPos, const QMouseEvent *event) {
    QVector<qreal> limits = {m_chartView->xMin, m_chartView->xMax, m_chartView->yMin, m_chartView->yMax};
    // Runs on the GUI thread; reuse the cached one-time snapshot of the points.
    if (m_points.size() != ptr->count()) {
        m_points = ptr->points();
    }
    Intercerp intersection = findIntersection(m_points, m_chartView->toggleFocus, chartPos, event->pos(), limits);
    // The Tooltip is displayed only if it is within a threshold distance
    // Get the X axis range for normalization
    qreal xRange = m_chartView->maxX - m_chartView->minX;
    qreal relativeThreshold = 0.001; // 0.1% of the total range

    if (intersection.isValid && xRange > std::numeric_limits<qreal>::epsilon() &&
        intersection.distance / xRange < relativeThreshold) {
        if (!intersection.pos.isNull()) {
            const QString tooltipText = QString("X: %1, Y: %2")
                    .arg(intersection.pos.x(), 0, 'f', 2)
                    .arg(intersection.pos.y(), 0, 'f', 2);

            QToolTip::showText(event->globalPosition().toPoint(), tooltipText);
            tooltipTimer->start(m_tooltipTimeout);
        }
    } else {
        QToolTip::hideText(); // Hides the previous tooltip immediately
    }
}

template<typename SeriesType>
Methods<SeriesType>::Intercerp
Methods<SeriesType>::findIntersection(const QList<QPointF> &points, const bool focusEnabled,
                                      const QPointF &chartPos, const QPointF &mousePos,
                                      const QVector<qreal> &limits) {
    // On GUI-thread snapshot, no race condition in worker thread.
    if (points.size() < 2) {
        return {};
    }

    const qreal mouseX = chartPos.x();
    const int pointCount = static_cast<int>(points.size());

    int left = 0;
    int right = pointCount - 1;
    const bool ascending = points.first().x() < points.last().x();

    while (left < right) {
        const int mid = left + (right - left) / 2;
        if (ascending) {
            if (points[mid].x() < mouseX) {
                left = mid + 1;
            } else {
                right = mid;
            }
        } else {
            if (points[mid].x() > mouseX) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
    }

    const int idx = std::max(0, left - 1);
    if (idx + 1 >= pointCount) {
        return {};
    }

    const QPointF &p1 = points[idx];
    const QPointF &p2 = points[idx + 1];

    const qreal minX = std::min(p1.x(), p2.x());
    const qreal maxX = std::max(p1.x(), p2.x());

    if (mouseX < minX || mouseX > maxX) {
        return {};
    }

    Intercerp result;
    result.IPpixel = mousePos;

    const qreal dx = p2.x() - p1.x();
    if (std::abs(dx) > std::numeric_limits<qreal>::epsilon()) {
        const qreal t = (mouseX - p1.x()) / dx;
        result.pos = QPointF(mouseX, p1.y() + t * (p2.y() - p1.y()));
        result.isValid = true;
        if (focusEnabled) {
            result.distance = distanceToLineSegment(chartPos, p1, p2);
        }
        return result;
    }
    return {};
}

template<typename SeriesType>
void Methods<SeriesType>::renderTracking(const TrackResult &result) {
    if (!result.isValid) {
        ptr->hideAll();
        return;
    }

    const QPointF intersectionPoint = result.pos;
    const QPointF mousePos = result.IPpixel; // IPpixel stores mousePos in background task

    const QPointF IPpixel = m_chartView->chart()->mapToPosition(intersectionPoint);

    QVector<qreal> limits = {m_chartView->xMin, m_chartView->xMax, m_chartView->yMin, m_chartView->yMax};

    if (!intersectionPoint.isNull() &&
        intersectionPoint.x() >= limits[0] && intersectionPoint.x() <= limits[1] &&
        intersectionPoint.y() >= limits[2] && intersectionPoint.y() <= limits[3]) {
        // Storing intersection point for further comparison
        m_chartView->updateIntersections(qobject_cast<QXYSeries *>(ptr), intersectionPoint);
        //
        updateVerticalLine(mousePos, IPpixel, limits); // Draw tracking lines
        setTooltips(intersectionPoint, IPpixel); // Creates the labels
        drawBullet(intersectionPoint); // Draw the bullet at the intersection
    } else {
        m_chartView->updateIntersections(qobject_cast<QXYSeries *>(ptr), QPointF());
        ptr->hideAll();
    }
}

Methods<SplineSeries>::Intercerp
SplineSeries::findIntersection(const QList<QPointF> &points, const bool focusEnabled,
                               const QPointF &chartPos, const QPointF &mousePos,
                               const QVector<qreal> &limits) {
    // On GUI-thread snapshot, no race condition in worker thread.
    const int pointCount = static_cast<int>(points.size());
    // Check if there are enough points
    if (pointCount < 2) {
        return {};
    }

    // Binary search to find the closest index point
    const qreal mouseX = chartPos.x();
    int left = 0;
    int right = pointCount - 1;
    bool ascending = points.first().x() < points.last().x();
    //
    while (left < right) {
        const int mid = left + (right - left) / 2;
        if (ascending) {
            if (points[mid].x() < mouseX) {
                left = mid + 1;
            } else {
                right = mid;
            }
        } else {
            if (points[mid].x() > mouseX) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
    }

    // Check having a valid segment
    const int idx = std::max(0, left - 1);
    if (idx + 1 >= pointCount) {
        return {};
    }

    // Get segment points
    const QPointF &p1 = points[idx];
    const QPointF &p2 = points[idx + 1];

    // Quick bounds check
    const qreal minX = std::min(p1.x(), p2.x());
    const qreal maxX = std::max(p1.x(), p2.x());

    if (mouseX < minX || mouseX > maxX) {
        return {};
    }

    Intercerp result;
    result.IPpixel = mousePos;

    // Calculate interpolation
    const qreal dx = p2.x() - p1.x();
    // Check division by zero
    if (std::abs(dx) > std::numeric_limits<qreal>::epsilon()) {
        QPointF interpolated;
        const qreal t = (mouseX - p1.x()) / dx;
        // Use Catmull-Rom if there are enough points
        if (idx > 0 && idx < pointCount - 2) {
            const QPointF &p0 = points[idx - 1];
            const QPointF &p3 = points[idx + 2];
            interpolated = catmullRomInterpolate(t, p0, p1, p2, p3);
        } else {
            // Fall back to linear interpolation for edge segments
            interpolated = QPointF(
                mouseX,
                p1.y() + t * (p2.y() - p1.y())
            );
        }

        result.pos = interpolated;
        result.isValid = true;
        if (focusEnabled) {
            // Only calculate distance if needed
            result.distance = distanceToLineSegment(chartPos, p1, interpolated);
        }
        return result;
    }
    // Return an empty object if no intersection found
    return {};
}

// Distance calculation from a point to a line segment
template<typename SeriesType>
qreal Methods<SeriesType>::distanceToLineSegment(const QPointF &point,
                                                 const QPointF &lineStart,
                                                 const QPointF &lineEnd) {
    // Cache coordinate differences
    const qreal dx = lineEnd.x() - lineStart.x();
    const qreal dy = lineEnd.y() - lineStart.y();

    // Check for a degenerate case (point segment)
    const qreal lengthSquared = dx * dx + dy * dy;
    if (lengthSquared < std::numeric_limits<qreal>::epsilon()) {
        // Point-to-point distance calculation
        const qreal px = point.x() - lineStart.x();
        const qreal py = point.y() - lineStart.y();
        return std::sqrt(px * px + py * py);
    }

    // Calculate projection parameter
    const qreal px = point.x() - lineStart.x();
    const qreal py = point.y() - lineStart.y();
    const qreal t = std::clamp((px * dx + py * dy) / lengthSquared, 0.0, 1.0);

    // Calculate the closest point on a line segment
    const qreal projX = lineStart.x() + t * dx;
    const qreal projY = lineStart.y() + t * dy;

    // Calculate distance
    const qreal distX = point.x() - projX;
    const qreal distY = point.y() - projY;
    return std::sqrt(distX * distX + distY * distY);
}

template<typename SeriesType>
void Methods<SeriesType>::createLines(int n) {
    lines.resize(n);
    // Populate the list with pointers
    std::generate_n(lines.begin(), n, []() {
        return new QGraphicsLineItem();
    });
};

template<typename SeriesType>
void Methods<SeriesType>::updateVerticalLine(
    // Update track-lines position
    const QPointF &mousePos, const QPointF &IPpixel, const QVector<qreal> &limits) {
    if (!m_chartView) return;
    m_chartView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    // --------
    QPointF chartPos = m_chartView->chart()->mapToValue(mousePos);
    if (chartPos.x() >= limits[0] && chartPos.x() <= limits[1] && chartPos.y() >= limits[2] &&
        chartPos.y() <= limits[3]) {
        if (lines.isEmpty()) {
            createLines(2);
            lines[0]->setPen(QPen(Qt::blue, 1, Qt::DashLine));
            lines[1]->setPen(QPen(Qt::blue, 1, Qt::DashLine));
            m_chartView->chart()->scene()->addItem(lines[0]);
            m_chartView->chart()->scene()->addItem(lines[1]);
        }

        lines[0]->show();
        lines[1]->show();

        // Track-line type switch: Crosshair | truncated
        if (m_chartView->toggleLines) {
            // Only show vertical track line belonging to the bottom-most placed series
            auto currentSeries = qobject_cast<QXYSeries *>(ptr);
            auto bottomSeries = m_chartView->findBottomSeries();
            //
            if (bottomSeries == currentSeries) {
                lines[0]->setLine(mousePos.x(),
                                  IPpixel.y(),
                                  mousePos.x(),
                                  m_chartView->chart()->plotArea().top());
                lines[0]->show();
            } else {
                lines[0]->hide();
            }

            // Always show the horizontal track line
            lines[1]->setLine(m_chartView->chart()->plotArea().left(),
                              IPpixel.y(),
                              mousePos.x(),
                              IPpixel.y());
        } else {
            lines[0]->setLine(mousePos.x(),
                              m_chartView->chart()->plotArea().bottom(),
                              mousePos.x(),
                              m_chartView->chart()->plotArea().top());

            lines[1]->setLine(m_chartView->chart()->plotArea().left(),
                              IPpixel.y(),
                              m_chartView->chart()->plotArea().right(),
                              IPpixel.y());
        }
    }
    m_chartView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
}

template<typename SeriesType>
void Methods<SeriesType>::createTooltips(const QList<TooltipData> &tooltipDataList) {
    const qsizetype n = tooltipDataList.size();
    if (toolTips.size() < n) {
        toolTips.resize(n);
        // Create QList of QLabels
        std::generate_n(toolTips.begin() + toolTips.size() - n, n, [this]() {
            return new QLabel(m_chartView);
        });
        // Create QList of QGraphicsDropShadowEffect
        shadowEffect.resize(n);
        std::generate_n(shadowEffect.begin() + shadowEffect.size() - n, n, [this]() {
            return new QGraphicsDropShadowEffect(m_chartView);
        });
        // Set a font for each label
        for (const auto tip: toolTips) {
            QFont font = tip->font();
            font.setBold(true);
            tip->setFont(font);
        }
        // Create a shadow effect for each label
        for (const auto effect: shadowEffect) {
            effect->setBlurRadius(10);
            effect->setXOffset(5);
            effect->setYOffset(5);
            effect->setColor(Qt::gray);
        }
    }

    for (qsizetype i = 0; i < n; ++i) {
        const auto &[text, position, color] = tooltipDataList[i];

        // Palette setup
        QPalette palette;
        palette.setColor(QPalette::Window, color);
        palette.setColor(QPalette::WindowText, QColor(255, 255, 255)); // White text

        toolTips[i]->setStyleSheet("border: 1.2px solid black; border-radius: 3px; padding: 3px;");
        toolTips[i]->setLineWidth(10);
        toolTips[i]->setAutoFillBackground(true);
        toolTips[i]->setPalette(palette);
        toolTips[i]->setGraphicsEffect(shadowEffect[i]);
        toolTips[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
        toolTips[i]->setWindowFlags(Qt::FramelessWindowHint);
        toolTips[i]->setText(text);
        toolTips[i]->setWordWrap(true);
        toolTips[i]->setMinimumSize(QSize(80, 20));
        toolTips[i]->setAlignment(Qt::AlignCenter);
        toolTips[i]->move(position);
        toolTips[i]->raise();
        toolTips[i]->show();
    }

    // Start or restart the custom tooltip (label) timer
    tooltipTimer->start(m_tooltipTimeout); // Hide all
}

template<typename SeriesType>
void Methods<SeriesType>::setTooltips(const QPointF &intersectionPoint, const QPointF &IPpixel) {
    // Cast IPpixel to QPoint
    const QPoint IPcd(static_cast<int>(IPpixel.x()), static_cast<int>(IPpixel.y()));
    //
    QList<TooltipData> tooltipDataList;
    // Create data for tooltips
    const auto IP = QPoint(IPcd.x() + 5, IPcd.y() + 5);
    const auto xLabel = QPoint(IPcd.x(), static_cast<int>(m_chartView->chart()->plotArea().top()) - 25);
    const auto yLabel = QPoint(static_cast<int>(m_chartView->chart()->plotArea().left()) - 60, IPcd.y());
    const QString tooltipText = QString("X: %1\nY: %2")
            .arg(intersectionPoint.x(), 0, 'f', 2)
            .arg(intersectionPoint.y(), 0, 'f', 2);

    // x axis label text
    const QString tooltipTextx = QString("X: %1").arg(intersectionPoint.x(), 0, 'f', 2);
    // y axis label text
    const QString tooltipTexty = QString("Y: %1").arg(intersectionPoint.y(), 0, 'f', 2);

    const auto *series = qobject_cast<SeriesType *>(ptr);
    const QColor colorSerie = series->pen().color();

    //------- x custom tooltip
    tooltipDataList.append({tooltipTextx, xLabel, Qt::black});
    //------- y custom tooltip
    tooltipDataList.append({tooltipTexty, yLabel, colorSerie});
    //------- Intersection-point custom tooltip
    tooltipDataList.append({tooltipText, IP, colorSerie});
    //-------

    // Create or update the tooltips
    createTooltips(tooltipDataList);
}

template<typename SeriesType>
void Methods<SeriesType>::drawBullet(const QPointF &point) {
    const QPointF pointPixel = m_chartView->chart()->mapToPosition(point);

    // Reuse a single ellipse item across updates: create it once, then just
    // reposition it. Avoids per-update scene churn (removeItem/delete/new/addItem).
    if (!bullet) {
        bullet = new QGraphicsEllipseItem(pointPixel.x() - 5,
                                          pointPixel.y() - 5,
                                          10,
                                          10); // 10x10 bullet
        bullet->setBrush(Qt::red); // Bullet color
        m_chartView->chart()->scene()->addItem(bullet);
    } else {
        bullet->setRect(pointPixel.x() - 5,
                        pointPixel.y() - 5,
                        10,
                        10);
        bullet->show();
    }
}

template<typename SeriesType>
void Methods<SeriesType>::hideTooltip() {
    if (!toolTips.isEmpty()) {
        for (const auto tip: toolTips) {
            tip->hide(); // Hide the tooltips
        }
    }
}

#include <QScopedPointer>

#include "CustomEvents.h"

ZoomAndScroll::ZoomAndScroll(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
      , toggleState(false)
      , toggleFocus(false)
      , toggleLines(false)
      , resizeZoom(false)
      , rubberBandItem(nullptr) {
    setMouseTracking(true); // Enable mouse tracking (runs once)
    minX = std::numeric_limits<qreal>::max();;
    maxX = std::numeric_limits<qreal>::lowest();
    minY = std::numeric_limits<qreal>::max();
    maxY = std::numeric_limits<qreal>::lowest();
}

void ZoomAndScroll::updateXLimits(const QChart *chart) {
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
    auto *xAxis = dynamic_cast<QValueAxis *>(chart()->axes(Qt::Horizontal).first());
    auto *yAxis = dynamic_cast<QValueAxis *>(chart()->axes(Qt::Vertical).first());

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
        toggleFocus = !toggleFocus; // Tooltips on cursor hover
    }
    if (event->key() == Qt::Key_R) {
        resizeZoom = !resizeZoom; // Horizontal panning (axis clipping)
    }
}

void ZoomAndScroll::mousePressEvent(QMouseEvent *event) {
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
        lastMousePos = event->pos(); // Update last mouse position
        setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(Qt::ClosedHandCursor);
    }
    rangeUpdate();
}

void ZoomAndScroll::clearRubberBand() {
    if (rubberBandItem) {
        rubberBandItem.reset(); // Automatic deletion
    }
}

void ZoomAndScroll::mouseReleaseEvent(QMouseEvent *event) {
    if (rubberBandItem) {
        QRectF rubberBandRect(rubberBandStartPos, event->pos());
        rubberBandRect = rubberBandRect.normalized();
        QRectF plotArea = chart()->plotArea();

        if (rubberBandRect.intersects(plotArea)) {
            chart()->zoomIn(rubberBandRect);
        }

        clearRubberBand(); // Remove rubber band
    }

    if (event->button() == Qt::RightButton) {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::ArrowCursor); // Reset cursor to default
    }
    rangeUpdate();
}

void ZoomAndScroll::mouseDoubleClickEvent(QMouseEvent *event) {
    event->accept();
    // Double-click to fit the chart with original axes ranges.
    if (event->button() == Qt::LeftButton) {
        // -----------------
        if (toggleState) {
            emit hide_when_move();
        }
        // -----------------
        resetChartToOriginal();
    }
    rangeUpdate();
}

void ZoomAndScroll::wheelEvent(QWheelEvent *event) {
    if (toggleState) {
        emit hide_when_move();
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

    auto *xAxis = qobject_cast<QValueAxis *>(chart()->axes()[0]);
    auto *yAxis = qobject_cast<QValueAxis *>(chart()->axes()[1]);

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
        if (resizeZoom) {
            if (numDegrees > 0) {
                newMinX = qMax(xMin, newMinX);
                newMaxX = qMin(xMax, newMaxX);
            } else {
                newMinX = qMin(xMin, newMinX);
                newMaxX = qMax(xMax, newMaxX);
            }
            xAxis->setRange(newMinX, newMaxX);
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

void ZoomAndScroll::mouseMoveEvent(QMouseEvent *event) {
    rangeUpdate();
    if (rubberBandItem) {
        // -----------------
        if (toggleState) {
            emit hide_when_move();
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
    }

    lastMousePos = event->pos(); // Update last mouse position

    const QPointF mousePos = mapToScene(event->pos());
    // Mouse position as emitted signal
    QVector<qreal> limits{xMin, xMax, yMin, yMax};
    if (!rubberBandItem) {
        emit mouseMoved(mousePos, event, limits);
    }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

LineSeries::LineSeries(ZoomAndScroll *chartView, QObject *parent)
    : QLineSeries(parent), Methods(this, chartView)
      , m_chartView(chartView) {
    // Signal/slot to transfer mouse move events to tracking method
    connect(m_chartView,
            &ZoomAndScroll::mouseMoved,
            this,
            &LineSeries::onMouseMoved);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hide_when_move,
            this,
            &LineSeries::hideAll);
    // Signal/slot to manage the timed hiding of labels
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer, &QTimer::timeout, this, &LineSeries::hideAll);
}

void LineSeries::hideAll() {
    if (!lines.isEmpty()) {
        lines[0]->hide();
        lines[1]->hide();
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
                              QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    }

    if (!m_chartView->toggleFocus && m_chartView->toggleState && isVisible) {
        handleTooltipForTracking(chartPos, mousePos, limits); // Tracking by line intersection
    } else {
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
            &ScatterSeries::onMouseMoved);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hide_when_move,
            this,
            &ScatterSeries::hideAll);
    // Signal/slot to manage the timed hiding of labels
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer, &QTimer::timeout, this, &ScatterSeries::hideAll);
}

void ScatterSeries::hideAll() {
    if (!lines.isEmpty()) {
        lines[0]->hide();
        lines[1]->hide();
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
                                 QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    }

    if (!m_chartView->toggleFocus && m_chartView->toggleState && isVisible) {
        handleTooltipForTracking(chartPos, mousePos, limits); // Tracking by line intersection
    } else {
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
            &SplineSeries::onMouseMoved);
    // Signal/slot for tracking's hiding effect during mouse events
    connect(m_chartView,
            &ZoomAndScroll::hide_when_move,
            this,
            &SplineSeries::hideAll);
    // Signal/slot to manage the timed hiding of labels
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer, &QTimer::timeout, this, &SplineSeries::hideAll);
}

void SplineSeries::hideAll() {
    if (!lines.isEmpty()) {
        lines[0]->hide();
        lines[1]->hide();
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
                                QVector<qreal> &limits) {
    const QPointF chartPos = chart()->mapToValue(mousePos);
    // Visibility checking condition
    const bool isVisible = chartPos.x() >= limits[0] && chartPos.x() <= limits[1]
                           && chartPos.y() >= limits[2] && chartPos.y() <= limits[3];

    if (m_chartView->toggleFocus && isVisible) {
        handleTooltipOnFocus(chartPos, event); // Labeling by mouse hovering
    }

    if (!m_chartView->toggleFocus && m_chartView->toggleState && isVisible) {
        handleTooltipForTracking(chartPos, mousePos, limits); // Tracking by line intersection
    } else {
        hideAll();
    }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename SeriesType>
Methods<SeriesType>::Methods(SeriesType *ptr, ZoomAndScroll *m_chartView)
    : bullet(nullptr),
      ptr(ptr),
      m_chartView(m_chartView) {
}

template<typename SeriesType>
void Methods<SeriesType>::deleteTooltip() {
    if (!toolTips.isEmpty()) {
        if (shadowEffect.data()) {
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
    QList<Intercerp> intersectionVector = findIntersection(chartPos);
    // The Tooltip is displayed only if it is within a threshold distance
    if (intersectionVector[0].distance < 0.5) {
        if (!intersectionVector[0].pos.isNull()) {
            const QString tooltipText = QString("X: %1, Y: %2")
                    .arg(intersectionVector[0].pos.x(), 0, 'f', 2)
                    .arg(intersectionVector[0].pos.y(), 0, 'f', 2);

            QToolTip::showText(event->globalPosition().toPoint(), tooltipText);
            tooltipTimer->start(1000); // Tooltip stays for 1000 ms (1 second)
        }
    } else {
        QToolTip::hideText(); // Hides previous tooltip immediately
    }
}

template<typename SeriesType>
QList<typename Methods<SeriesType>::Intercerp>
Methods<SeriesType>::findIntersection(const QPointF mouse) const {
    const auto *series = qobject_cast<SeriesType *>(ptr);
    if (!series || series->count() < 2) {
        return {}; // Return an empty list if no series or not enough points
    }

    constexpr qreal adjpoints = 2.0; // Number of adjacent points
    int startIdx = 0;
    int endIdx = series->count() - 1;
    // Binary search for the closest index to the right of x, and gives a starting index
    while (startIdx < endIdx) {
        int midIdx = (startIdx + endIdx) / 2;
        if (series->at(midIdx).x() < mouse.x()) {
            startIdx = midIdx + 1;
        } else {
            endIdx = midIdx;
        }
    }
    // Search Interval is limited to a small number of neighboring points around x-coordinate
    const int closestIndex = startIdx;
    int searchStartIndex = std::max(0, closestIndex - 1);
    int searchEndIndex = std::min(series->count() - 1, closestIndex + 1);

    // Expand the search interval according to adjacent points number (adjpoints)
    while (searchStartIndex > 0 &&
           series->at(searchStartIndex - 1).x() >= mouse.x() - adjpoints) {
        --searchStartIndex;
    }
    while (searchEndIndex < series->count() - 1 &&
           series->at(searchEndIndex + 1).x() <= mouse.x() + adjpoints) {
        ++searchEndIndex;
    }

    // Linear Interpolation provides a precise intersection point between the two data points
    QList<Intercerp> points;

    // Check the line segments within the limited range
    for (int i = searchStartIndex; i < searchEndIndex; ++i) {
        QPointF p1 = series->at(i);
        QPointF p2 = series->at(i + 1);

        // Check if x is within the bounds of p1 and p2
        if ((mouse.x() >= p1.x() && mouse.x() <= p2.x()) ||
            (mouse.x() <= p1.x() && mouse.x() >= p2.x())) {
            // Linear interpolation to find the corresponding y value
            const qreal y = p1.y() + (mouse.x() - p1.x()) * (p2.y() - p1.y()) / (p2.x() - p1.x());
            if (m_chartView->toggleFocus) {
                const qreal distance = distanceToLineSegment(mouse, p1, p2);
                points.append({distance, p1});
            } else {
                points.append({0, QPointF(mouse.x(), y)});
            }
            return points; // Return found intersection
        }
    }
    // Return an empty list if no intersection found
    return {};
}

// Distance calculation from a point to a line segment
template<typename SeriesType>
qreal Methods<SeriesType>::distanceToLineSegment(const QPointF &point,
                                                 const QPointF &lineStart,
                                                 const QPointF &lineEnd) {
    const qreal dx = lineEnd.x() - lineStart.x();
    const qreal dy = lineEnd.y() - lineStart.y();
    if ((dx == 0) && (dy == 0)) {
        // Special case: the segment is a point
        return QLineF(point, lineStart).length();
    }

    // Calculate the projection of the point onto the line segment
    const qreal t = ((point.x() - lineStart.x()) * dx + (point.y() - lineStart.y()) * dy) /
                    (dx * dx + dy * dy);

    if (t < 0)
        return QLineF(point, lineStart).length(); // Closest to lineStart
    if (t > 1)
        return QLineF(point, lineEnd).length(); // Closest to lineEnd

    // Find the point on the line segment
    const auto projection = QPointF(lineStart.x() + t * dx, lineStart.y() + t * dy);
    return QLineF(point, projection).length();
}

// Tracking by line intersection
template<typename SeriesType>
void Methods<SeriesType>::handleTooltipForTracking(
    const QPointF &chartPos, const QPointF &mousePos, QVector<qreal> &limits) {
    // Find intersection with the series
    QList<Intercerp> intersectionVector = findIntersection(chartPos);
    if (intersectionVector.isEmpty()) {
        return;
    };
    const QPointF intersectionPoint = intersectionVector[0].pos;
    const QPointF IPpixel = m_chartView->chart()->mapToPosition(intersectionPoint);
    if (!intersectionPoint.isNull() && intersectionPoint.y() <= limits[3] &&
        intersectionPoint.y() >= limits[2]) {
        updateVerticalLine(mousePos, IPpixel, limits); // Draw tracking lines
        setTooltips(intersectionPoint, IPpixel); // Creates the labels
        drawBullet(intersectionPoint); // Draw the bullet at the intersection
    } else {
        ptr->hideAll();
    }
}

template<typename SeriesType>
void Methods<SeriesType>::createLines(int n) {
    // Resize the QList to hold n track-lines
    lines.resize(n);

    // Populate the list with pointers
    std::generate_n(lines.begin(), n, []() {
        return new QGraphicsLineItem();
    });
};

template<typename SeriesType>
void Methods<SeriesType>::updateVerticalLine(
    // Update track-lines position
    const QPointF &mousePos, const QPointF &IPpixel, QVector<qreal> &limits) {
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
            lines[0]->setLine(mousePos.x(), IPpixel.y(), mousePos.x(), m_chartView->chart()->plotArea().top());
            lines[1]->setLine(m_chartView->chart()->plotArea().left(), IPpixel.y(), IPpixel.x(), IPpixel.y());
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
        // Set font for each label
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

    for (int i = 0; i < n; ++i) {
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
    tooltipTimer->start(1000); // Hide all after 1 second (1000 ms)
}

template<typename SeriesType>
void Methods<SeriesType>::setTooltips(const QPointF &intersectionPoint, const QPointF &IPpixel) {
    // Cast IPpixel to QPoint
    const QPoint IPcd(static_cast<int>(IPpixel.x()), static_cast<int>(IPpixel.y()));

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

    // Create data for tooltips
    QList<TooltipData> tooltipDataList;

    const auto *series = qobject_cast<SeriesType *>(ptr);
    const QColor colorSerie = series->pen().color(); // Get the color from the series

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
    // Remove existing intersection bullet symbol
    if (bullet) {
        m_chartView->chart()->scene()->removeItem(bullet);
        delete bullet;
        bullet = nullptr; // Avoiding dangling pointer
    }
    const QPointF pointPixel = m_chartView->chart()->mapToPosition(point);

    // Create a new bullet centered at the intersection point
    bullet = new QGraphicsEllipseItem(pointPixel.x() - 5,
                                      pointPixel.y() - 5,
                                      10,
                                      10); // 10x10 bullet
    bullet->setBrush(Qt::red); // Bullet color
    m_chartView->chart()->scene()->addItem(bullet);
}

template<typename SeriesType>
void Methods<SeriesType>::hideTooltip() {
    if (!toolTips.isEmpty()) {
        for (const auto tip: toolTips) {
            tip->hide(); // Hide the tooltips
        }
    }
}

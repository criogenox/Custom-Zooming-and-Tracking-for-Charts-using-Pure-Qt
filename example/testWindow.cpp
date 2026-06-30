#include "testWindow.h"
#include "customEvents.h"
#include <QValueAxis>

testWindow::testWindow(QWidget *parent)
    : QMainWindow(parent) {
    auto *chart = new QChart();

    // Custom classes ***************************
    auto *chartView = new ZoomAndScroll(chart);
    auto *series1 = new LineSeries(chartView);
    auto *series2 = new ScatterSeries(chartView);
    auto *series3 = new SplineSeries(chartView);
    //*******************************************

    // Dense, steady plot
    constexpr int maxPoints = 10000;
    for (int i = -10000; i < maxPoints; ++i) {
        qreal j = i * 0.1;
        series1->append(j, -1 * sin(j) * 5);
        series2->append(j, cos(j) * 3 + 20);
        series3->append(j, sin(j * 0.5) * 4 + 30);
    }
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(series3);

    // Setting global chart limits
    chartView->updateXLimits(chart);

    // Series setting
    // -------------
    QPen shadowPen1(Qt::blue);
    shadowPen1.setWidth(3);
    shadowPen1.setStyle(Qt::SolidLine);
    shadowPen1.setCapStyle(Qt::RoundCap);
    shadowPen1.setJoinStyle(Qt::RoundJoin);
    series1->setPen(shadowPen1);
    series1->setPointLabelsFormat("@xPoint, @yPoint");
    series1->setName("Acquired signal");
    // -------------
    QPen shadowPen2(Qt::darkGreen);
    // shadowPen2.setWidth(3);
    // shadowPen2.setStyle(Qt::DashLine);
    series2->setPen(shadowPen2);
    series2->setMarkerShape(QScatterSeries::MarkerShapeStar);
    series2->setMarkerSize(10);
    // series2->setBorderColor(Qt::blue);
    series2->setPointLabelsFormat("@xPoint, @yPoint");
    series2->setName("Simulated vibrations");
    // -------------
    QPen shadowPen3(Qt::red);
    shadowPen3.setWidth(3);
    shadowPen3.setStyle(Qt::DashLine);
    series3->setPen(shadowPen3);
    series3->setPointLabelsFormat("@xPoint, @yPoint");
    series3->setName("Predicted vibrations");

    // Chart axes setting
    chart->createDefaultAxes();
    constexpr QColor axisColor(200, 200, 200); // Color (axes & grid lines)
    // Axes labeling
    std::pair<Qt::Orientation, QString> axesInfo[] = {
        {Qt::Horizontal, "Time [seconds]"},
        {Qt::Vertical, "Frequency [Hz]"}
    };

    for (const auto &[fst, snd]: axesInfo) {
        const Qt::Orientation orientation = fst;
        QString title = snd;

        auto axes = chart->axes(orientation);
        if (!axes.isEmpty()) {
            if (auto *axis = qobject_cast<QValueAxis *>(axes[0])) {
                QPen axisPen(axisColor);
                axis->setLinePen(axisPen);
                axis->setTickCount(11);
                axis->setGridLinePen(axisColor);
                // Setting axes ranges
                if (orientation == Qt::Horizontal) {
                    qreal xRange = chartView->maxX - chartView->minX;
                    axis->setRange(chartView->minX, chartView->minX + xRange);
                } else if (orientation == Qt::Vertical) {
                    axis->setRange(chartView->minY, chartView->maxY);
                }
                axis->setTitleText(title);
            }
        }
    }
    chartView->rangeUpdate();

    chart->setTitle("Data Spectrum Analysis  (mock testing example)");
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));

    // Background color gradient
    QLinearGradient gradient(0, 0, 0, 700);
    gradient.setColorAt(0, QColor(255, 255, 255));
    gradient.setColorAt(0.5, QColor(235, 250, 255));
    gradient.setColorAt(1, QColor(162, 210, 232));
    chart->setBackgroundBrush(QBrush(gradient));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setFont(QFont("Arial", 10));

    setCentralWidget(chartView);
}

#include <QDebug>
#include <QVBoxLayout>
#include "TestWindow.h"
#include "CustomChartUtils.h"
#include "CustomEvents.h"

TestWindow::TestWindow(QWidget *parent)
    : QMainWindow(parent) {
    //#######################
    auto *chart = new QChart();

    // Custom classes ***************************
    auto *chartView = new ZoomAndScroll(chart);
    auto *series1 = new TrackingSeries(chartView);
    auto *series2 = new TrackingSeries(chartView);
    auto *series3 = new TrackingSeries(chartView);
    //*******************************************

    //  Series of random data
    for (int i = 0; i < 200; ++i) {
        qreal yValue = rand() % 6;
        qreal j = 2 * i;
        qreal _yValue = 0.9 * yValue;
        series1->append(j, _yValue);
    }
    for (int i = 0; i < 200; ++i) {
        qreal yValue = rand() % 4 + 15;
        qreal j = 2 * i;
        qreal _yValue = 0.7 * yValue;
        series2->append(j, _yValue);
    }
    for (int i = 0; i < 200; ++i) {
        qreal yValue = rand() % 8 + 30;
        qreal j = 2 * i;
        qreal _yValue = 0.7 * yValue;
        series3->append(j, _yValue);
    }
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(series3);
    // Setting global chart limits
    ChartUtils::updateXLimits(chart);

    //#######################

    // Series setting
    QPen shadowPen1(Qt::blue);
    shadowPen1.setWidth(3);
    shadowPen1.setStyle(Qt::DashLine);
    QPen shadowPen2(Qt::darkGreen);
    shadowPen2.setWidth(3);
    shadowPen2.setStyle(Qt::DashLine);
    QPen shadowPen3(Qt::red);
    shadowPen3.setWidth(3);
    shadowPen3.setStyle(Qt::DashLine);
    series1->setPen(shadowPen1);
    series1->setPointLabelsFormat("@xPoint, @yPoint");
    series1->setName("Acquired signal");
    series2->setPen(shadowPen2);
    series2->setPointLabelsFormat("@xPoint, @yPoint");
    series2->setName("Simulated vibrations");
    series3->setPen(shadowPen3);
    series3->setPointLabelsFormat("@xPoint, @yPoint");
    series3->setName("Predicted vibrations");
    //#######################

    // Chart axes setting
    chart->createDefaultAxes();
    QColor axisColor(200, 200, 200); // Color (axes & grid lines)
    // Axes labeling
    std::pair<Qt::Orientation, QString> axesInfo[] = {
        {Qt::Horizontal, "Time [seconds]"},
        {Qt::Vertical, "Frequency [Hz]"}
    };

    for (const auto &[fst, snd]: axesInfo) {
        Qt::Orientation orientation = fst;
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
                    axis->setRange(ChartUtils::minX, ChartUtils::maxX);
                } else if (orientation == Qt::Vertical) {
                    axis->setRange(ChartUtils::minY, ChartUtils::maxY);
                }
                axis->setTitleText(title);
            }
        }
    }

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
    // chart->setAnimationOptions(QChart::AllAnimations);

    //#######################

    //// Set up layout for multi chart views
    // auto *layout= new QVBoxLayout();
    // layout->addWidget(chartView0);
    // layout->addWidget(chartView1);
    // auto *central_widget = new QWidget(this);
    // central_widget->setLayout(layout);

    //#######################

    setCentralWidget(chartView); // or use central_widget
}

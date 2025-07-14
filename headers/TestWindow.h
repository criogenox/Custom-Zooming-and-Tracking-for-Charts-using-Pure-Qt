#ifndef TESTWINDOW_H
#define TESTWINDOW_H
#pragma once

#include <QMainWindow>

class TestWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit TestWindow(QWidget *parent = nullptr);

    ~TestWindow() override = default;
};

#endif // TESTWINDOW_H

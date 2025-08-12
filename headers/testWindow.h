#ifndef TESTWINDOW_H
#define TESTWINDOW_H
#pragma once

#include <QMainWindow>

class testWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit testWindow(QWidget *parent = nullptr);

    ~testWindow() override = default;
};

#endif // TESTWINDOW_H

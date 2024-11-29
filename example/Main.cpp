#include <QApplication>

#include "TestWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestWindow window;
    // window.resize(800, 600);
    window.showMaximized();
    return QApplication::exec();
}

#include <QApplication>

#include "testWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    testWindow window;
    // window.resize(800, 600);
    window.showMaximized();
    return QApplication::exec();
}

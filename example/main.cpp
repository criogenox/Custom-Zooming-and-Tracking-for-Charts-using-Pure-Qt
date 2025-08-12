#include <QApplication>

#include "testWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    testWindow window;

    window.showMaximized();
    return QApplication::exec();
}

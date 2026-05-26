#ifndef FILEMENU_H
#define FILEMENU_H

#include <QObject>

class FileMenu : public QObject
{
    Q_OBJECT

public:
    explicit FileMenu(QObject *parent = nullptr);

public slots:
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileExport();
    void onFilePrint();
    void onFileExit();
    void onHelpAbout();
    void onHelpAboutQt();
    void onHelpContents();

private:
    void showNotImplemented(const QString &actionName);
};

#endif // FILEMENU_H

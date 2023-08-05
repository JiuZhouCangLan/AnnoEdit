#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QTextEdit>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void addFile(const QString& filePath);
    void saveCurrent();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void initUI();
    void initSlot();
    QTextEdit *openFile(const QString& filePath);
    bool saveFile(const QString& content, const QString& filePath);

    Ui::MainWindow *ui;
    QHash<QString, QTreeWidgetItem*> m_itemMap;
};
#endif // MAINWINDOW_H

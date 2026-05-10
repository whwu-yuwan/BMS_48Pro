#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCanBus>       // 引入Qt CAN模块头文件
#include <QCanBusDevice> // CAN设备核心类
#include <QHash>
#include <QSerialPort>
#include <QTimer>
#include <QVariant>
#include <QString>

using CanConfigMap = QHash<QCanBusDevice::ConfigurationKey, QVariant>;

extern QString g_canPlugin;
extern QString g_canInterfaceName;
extern CanConfigMap g_canConfig;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_dischargemos_open_2_clicked();

    void on_dischargemos_close_2_clicked();

    void on_can_connect_clicked();

    void on_can_disconnect_clicked();

    void on_dischargemos_open_clicked();

    void on_dischargemos_close_clicked();

    void on_chargemos_close_clicked();

    void on_chargemos_open_clicked();

    void on_comboBox_activated(const QString &arg1);

private:
    Ui::MainWindow *ui;
    void initUI(); // 初始化UI控件（下拉框选项、按钮状态）
    bool connectCan();
    bool connectSerialCan();
    void disconnectCan();
    void appendLog(const QString &message);
    bool sendSlcanCommand(const QByteArray &command, int timeoutMs = 300, bool acceptNoReply = false);
    QByteArray slcanBitrateCode(int bitrate) const;
    bool sendMosControlFrame(quint8 chargeMos, quint8 dischargeMos);
    void handleSerialReadyRead();
    void parseAndLogSlcanFrame(const QByteArray &line);
    void setCanUiConnected(bool connected);
    QCanBusDevice *m_canDevice = nullptr;
    QSerialPort *m_serialPort = nullptr;
    QByteArray m_slcanRxBuffer;
    quint8 m_chargeMosState = 0;
    quint8 m_dischargeMosState = 0;
    bool m_waitingMosAck = false;
    quint8 m_pendingChargeMos = 0;
    quint8 m_pendingDischargeMos = 0;
};
#endif // MAINWINDOW_H

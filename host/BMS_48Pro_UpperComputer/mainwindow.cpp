#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCanBusFrame>
#include <QDateTime>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QSet>
#include <QTimer>
#include <QVariantMap>

QString g_canPlugin;
QString g_canInterfaceName;
CanConfigMap g_canConfig{
    {QCanBusDevice::BitRateKey, 500000},
    {QCanBusDevice::CanFdKey, false},
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
}

MainWindow::~MainWindow()
{
    disconnectCan();
    delete ui;
}

void MainWindow::initUI()
{
    ui->comboBox->clear();
    ui->comboBox_2->clear();
    ui->comboBox_3->clear();

    QSet<QString> addedKeys;
    QStringList plugins;
    if (QCanBus::instance()) {
        plugins = QCanBus::instance()->plugins();
    }
    const bool slcanAvailable = plugins.contains(QStringLiteral("slcan"));

    if (QCanBus::instance()) {
        for (const QString &plugin : plugins) {
            const auto deviceInfos = QCanBus::instance()->availableDevices(plugin);
            for (const auto &info : deviceInfos) {
                const QString interfaceName = info.name();
                const QString key = plugin + QStringLiteral("|") + interfaceName;
                if (addedKeys.contains(key)) {
                    continue;
                }
                addedKeys.insert(key);
                const QVariantMap data{
                    {QStringLiteral("plugin"), plugin},
                    {QStringLiteral("interface"), interfaceName},
                };
                ui->comboBox->addItem(plugin + QStringLiteral(" | ") + interfaceName, data);
            }
        }
    }

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        const QString portName = port.portName();
        QString display = QStringLiteral("Serial | ") + portName;

        QStringList extras;
        if (!port.description().isEmpty()) {
            extras << port.description();
        }
        if (!port.manufacturer().isEmpty()) {
            extras << port.manufacturer();
        }
        if (!extras.isEmpty()) {
            display += QStringLiteral(" | ") + extras.join(QStringLiteral(" | "));
        }

        QString plugin = slcanAvailable ? QStringLiteral("slcan") : QString();
        if (!slcanAvailable) {
            display += QStringLiteral(" | 未检测到slcan插件");
        }

        const QString key = (plugin.isEmpty() ? QStringLiteral("serial") : plugin) + QStringLiteral("|") + portName;
        if (addedKeys.contains(key)) {
            continue;
        }
        addedKeys.insert(key);

        const QVariantMap data{
            {QStringLiteral("plugin"), plugin},
            {QStringLiteral("interface"), portName},
        };
        ui->comboBox->addItem(display, data);
    }

    if (ui->comboBox->count() == 0) {
        const QVariantMap data{
            {QStringLiteral("plugin"), QString()},
            {QStringLiteral("interface"), QString()},
        };
        ui->comboBox->addItem(QStringLiteral("未发现CAN/串口设备"), data);
    }

    ui->comboBox_2->addItem(QStringLiteral("125000"), 125000);
    ui->comboBox_2->addItem(QStringLiteral("250000"), 250000);
    ui->comboBox_2->addItem(QStringLiteral("500000"), 500000);
    ui->comboBox_2->addItem(QStringLiteral("1000000"), 1000000);
    ui->comboBox_2->setCurrentIndex(2);

    ui->comboBox_3->addItem(QStringLiteral("CAN 2.0"), false);
    ui->comboBox_3->addItem(QStringLiteral("CAN FD"), true);
    ui->comboBox_3->setCurrentIndex(0);

    auto syncGlobals = [this]() {
        const QVariantMap data = ui->comboBox->currentData().toMap();
        g_canPlugin = data.value(QStringLiteral("plugin")).toString();
        g_canInterfaceName = data.value(QStringLiteral("interface")).toString();
        g_canConfig[QCanBusDevice::BitRateKey] = ui->comboBox_2->currentData();
        g_canConfig[QCanBusDevice::CanFdKey] = ui->comboBox_3->currentData();
    };

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncGlobals](int) {
        syncGlobals();
    });
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncGlobals](int) {
        syncGlobals();
    });
    connect(ui->comboBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncGlobals](int) {
        syncGlobals();
    });

    syncGlobals();
    setCanUiConnected(false);
    appendLog(QStringLiteral("程序启动，设备扫描完成"));
}

void MainWindow::appendLog(const QString &message)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    ui->rizhi->appendPlainText(QStringLiteral("[%1] %2").arg(ts, message));
}

void MainWindow::setCanUiConnected(bool connected)
{
    ui->can_connect->setEnabled(!connected);
    ui->can_disconnect->setEnabled(connected);
    ui->comboBox->setEnabled(!connected);
    ui->comboBox_2->setEnabled(!connected);
    ui->comboBox_3->setEnabled(!connected);
}

bool MainWindow::connectCan()
{
    appendLog(QStringLiteral("开始连接：plugin=%1 interface=%2 baud=%3 canfd=%4")
              .arg(g_canPlugin,
                   g_canInterfaceName,
                   g_canConfig.value(QCanBusDevice::BitRateKey).toString(),
                   g_canConfig.value(QCanBusDevice::CanFdKey).toBool() ? QStringLiteral("on") : QStringLiteral("off")));

    if (m_canDevice || m_serialPort) {
        if (m_serialPort && m_serialPort->isOpen()) {
            setCanUiConnected(true);
            appendLog(QStringLiteral("连接复用：串口CAN已是连接状态"));
            return true;
        }
        if (m_canDevice && m_canDevice->state() == QCanBusDevice::ConnectedState) {
            setCanUiConnected(true);
            appendLog(QStringLiteral("连接复用：Qt CAN已是连接状态"));
            return true;
        }
        disconnectCan();
    }

    if (g_canInterfaceName.isEmpty()) {
        appendLog(QStringLiteral("连接失败：未选择接口"));
        QMessageBox::warning(this, QStringLiteral("CAN"), QStringLiteral("未选择CAN插件或接口"));
        return false;
    }
    if (g_canPlugin.isEmpty()) {
        appendLog(QStringLiteral("进入自定义串口CAN连接流程"));
        return connectSerialCan();
    }
    if (!QCanBus::instance()) {
        appendLog(QStringLiteral("连接失败：Qt SerialBus 未初始化"));
        QMessageBox::warning(this, QStringLiteral("CAN"), QStringLiteral("Qt SerialBus 未初始化"));
        return false;
    }

    QString errorString;
    m_canDevice = QCanBus::instance()->createDevice(g_canPlugin, g_canInterfaceName, &errorString);
    if (!m_canDevice) {
        appendLog(QStringLiteral("连接失败：创建Qt CAN设备失败，%1").arg(errorString));
        QMessageBox::warning(this, QStringLiteral("CAN"), errorString.isEmpty() ? QStringLiteral("创建CAN设备失败") : errorString);
        return false;
    }

    for (auto it = g_canConfig.cbegin(); it != g_canConfig.cend(); ++it) {
        m_canDevice->setConfigurationParameter(it.key(), it.value());
    }

    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, [this](QCanBusDevice::CanBusError) {
        if (!m_canDevice) {
            return;
        }
        if (m_canDevice->error() == QCanBusDevice::NoError) {
            return;
        }
        appendLog(QStringLiteral("Qt CAN错误：%1").arg(m_canDevice->errorString()));
        QMessageBox::warning(this, QStringLiteral("CAN"), m_canDevice->errorString());
    });
    connect(m_canDevice, &QCanBusDevice::stateChanged, this, [this](QCanBusDevice::CanBusDeviceState state) {
        setCanUiConnected(state == QCanBusDevice::ConnectedState);
        appendLog(QStringLiteral("Qt CAN状态变化：%1").arg(QString::number(static_cast<int>(state))));
    });

    if (!m_canDevice->connectDevice()) {
        const QString err = m_canDevice->errorString();
        m_canDevice->deleteLater();
        m_canDevice = nullptr;
        setCanUiConnected(false);
        appendLog(QStringLiteral("连接失败：%1").arg(err));
        QMessageBox::warning(this, QStringLiteral("CAN"), err.isEmpty() ? QStringLiteral("连接CAN失败") : err);
        return false;
    }

    setCanUiConnected(true);
    appendLog(QStringLiteral("Qt CAN连接成功"));
    return true;
}

QByteArray MainWindow::slcanBitrateCode(int bitrate) const
{
    switch (bitrate) {
    case 10000: return QByteArrayLiteral("S0\r");
    case 20000: return QByteArrayLiteral("S1\r");
    case 50000: return QByteArrayLiteral("S2\r");
    case 100000: return QByteArrayLiteral("S3\r");
    case 125000: return QByteArrayLiteral("S4\r");
    case 250000: return QByteArrayLiteral("S5\r");
    case 500000: return QByteArrayLiteral("S6\r");
    case 750000: return QByteArrayLiteral("S7\r");
    case 1000000: return QByteArrayLiteral("S8\r");
    default: return QByteArray();
    }
}

bool MainWindow::sendMosControlFrame(quint8 chargeMos, quint8 dischargeMos)
{
    chargeMos = chargeMos ? 1 : 0;
    dischargeMos = dischargeMos ? 1 : 0;
    const QByteArray payload = QByteArray::fromRawData(reinterpret_cast<const char *>(&chargeMos), 1)
            + QByteArray::fromRawData(reinterpret_cast<const char *>(&dischargeMos), 1);

    if (m_serialPort && m_serialPort->isOpen()) {
        const QString chargeHex = QStringLiteral("%1")
                .arg(chargeMos, 2, 16, QLatin1Char('0'))
                .toUpper();
        const QString dischargeHex = QStringLiteral("%1")
                .arg(dischargeMos, 2, 16, QLatin1Char('0'))
                .toUpper();
        const QByteArray slcan = (QStringLiteral("t3242") + chargeHex + dischargeHex + QStringLiteral("\r")).toLatin1();
        const bool ok = sendSlcanCommand(slcan, 300, true);
        appendLog(QStringLiteral("TX MOS(串口) id=0x324 charge=%1 discharge=%2 adapter=%3 raw=%4")
                  .arg(QString::number(chargeMos),
                       QString::number(dischargeMos),
                       ok ? QStringLiteral("ok") : QStringLiteral("fail"),
                       QString::fromLatin1(slcan.trimmed())));
        if (ok) {
            m_waitingMosAck = true;
            m_pendingChargeMos = chargeMos;
            m_pendingDischargeMos = dischargeMos;
            QTimer::singleShot(500, this, [this]() {
                if (!m_waitingMosAck) {
                    return;
                }
                m_waitingMosAck = false;
                appendLog(QStringLiteral("MOS回执超时：500ms内未收到对端响应(0x324)"));
            });
        }
        return ok;
    }

    if (m_canDevice && m_canDevice->state() == QCanBusDevice::ConnectedState) {
        QCanBusFrame frame(0x324, payload);
        frame.setFrameType(QCanBusFrame::DataFrame);
        frame.setExtendedFrameFormat(false);
        const bool ok = m_canDevice->writeFrame(frame);
        appendLog(QStringLiteral("TX MOS(QtCAN) id=0x324 charge=%1 discharge=%2 adapter=%3")
                  .arg(QString::number(chargeMos),
                       QString::number(dischargeMos),
                       ok ? QStringLiteral("ok") : QStringLiteral("fail")));
        if (ok) {
            m_waitingMosAck = true;
            m_pendingChargeMos = chargeMos;
            m_pendingDischargeMos = dischargeMos;
            QTimer::singleShot(500, this, [this]() {
                if (!m_waitingMosAck) {
                    return;
                }
                m_waitingMosAck = false;
                appendLog(QStringLiteral("MOS回执超时：500ms内未收到对端响应(0x324)"));
            });
        }
        return ok;
    }

    appendLog(QStringLiteral("TX MOS失败：CAN未连接"));
    QMessageBox::warning(this, QStringLiteral("CAN"), QStringLiteral("请先连接CAN"));
    return false;
}

bool MainWindow::sendSlcanCommand(const QByteArray &command, int timeoutMs, bool acceptNoReply)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return false;
    }
    m_serialPort->clearError();
    if (m_serialPort->write(command) != command.size()) {
        return false;
    }
    if (!m_serialPort->waitForBytesWritten(timeoutMs)) {
        return false;
    }
    if (acceptNoReply) {
        return true;
    }

    // NOTE: Do not read all input here. RX parsing is handled by readyRead path.
    if (!m_serialPort->waitForReadyRead(timeoutMs)) {
        return false;
    }
    return true;
}

bool MainWindow::connectSerialCan()
{
    if (g_canConfig.value(QCanBusDevice::CanFdKey).toBool()) {
        appendLog(QStringLiteral("连接失败：SLCAN不支持CAN FD"));
        QMessageBox::warning(this, QStringLiteral("CAN"), QStringLiteral("SLCAN串口模式不支持CAN FD"));
        return false;
    }

    const int bitrate = g_canConfig.value(QCanBusDevice::BitRateKey, 500000).toInt();
    const QByteArray bitrateCmd = slcanBitrateCode(bitrate);
    if (bitrateCmd.isEmpty()) {
        appendLog(QStringLiteral("连接失败：SLCAN不支持波特率 %1").arg(QString::number(bitrate)));
        QMessageBox::warning(this, QStringLiteral("CAN"), QStringLiteral("SLCAN不支持该波特率"));
        return false;
    }

    m_serialPort = new QSerialPort(this);
    m_slcanRxBuffer.clear();
    m_serialPort->setPortName(g_canInterfaceName);
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        const QString err = m_serialPort->errorString();
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
        appendLog(QStringLiteral("连接失败：串口打开失败，%1").arg(err));
        QMessageBox::warning(this, QStringLiteral("CAN"), err.isEmpty() ? QStringLiteral("串口打开失败") : err);
        return false;
    }
    appendLog(QStringLiteral("串口打开成功：%1 @115200").arg(g_canInterfaceName));

    if (!sendSlcanCommand(QByteArrayLiteral("C\r"), 300, true)
        || !sendSlcanCommand(bitrateCmd, 300, true)
        || !sendSlcanCommand(QByteArrayLiteral("O\r"), 300, true)) {
        const QString err = m_serialPort->errorString();
        m_serialPort->close();
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
        appendLog(QStringLiteral("连接失败：SLCAN初始化失败，%1").arg(err));
        QMessageBox::warning(this, QStringLiteral("CAN"), err.isEmpty() ? QStringLiteral("SLCAN初始化失败，请确认设备协议") : err);
        return false;
    }

    setCanUiConnected(true);
    connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::handleSerialReadyRead);
    appendLog(QStringLiteral("自定义串口CAN连接成功"));
    return true;
}

void MainWindow::handleSerialReadyRead()
{
    if (!m_serialPort) {
        return;
    }
    m_slcanRxBuffer += m_serialPort->readAll();
    int endPos = m_slcanRxBuffer.indexOf('\r');
    while (endPos >= 0) {
        const QByteArray line = m_slcanRxBuffer.left(endPos);
        m_slcanRxBuffer.remove(0, endPos + 1);
        if (!line.isEmpty()) {
            parseAndLogSlcanFrame(line);
        }
        endPos = m_slcanRxBuffer.indexOf('\r');
    }
}

void MainWindow::parseAndLogSlcanFrame(const QByteArray &line)
{
    if (line.isEmpty()) {
        return;
    }

    const char frameType = line.at(0);
    bool ok = false;
    uint frameId = 0;
    int dlc = 0;
    QByteArray payload;

    if (frameType == 't' || frameType == 'r') {
        if (line.size() < 5) {
            return;
        }
        frameId = line.mid(1, 3).toUInt(&ok, 16);
        if (!ok) {
            return;
        }
        dlc = QString(line.mid(4, 1)).toInt(&ok, 16);
        if (!ok) {
            return;
        }
        if (frameType == 't') {
            const int expectedLen = 5 + dlc * 2;
            if (line.size() < expectedLen) {
                return;
            }
            payload = line.mid(5, dlc * 2).toUpper();
        }
    } else if (frameType == 'T' || frameType == 'R') {
        if (line.size() < 10) {
            return;
        }
        frameId = line.mid(1, 8).toUInt(&ok, 16);
        if (!ok) {
            return;
        }
        dlc = QString(line.mid(9, 1)).toInt(&ok, 16);
        if (!ok) {
            return;
        }
        if (frameType == 'T') {
            const int expectedLen = 10 + dlc * 2;
            if (line.size() < expectedLen) {
                return;
            }
            payload = line.mid(10, dlc * 2).toUpper();
        }
    } else {
        return;
    }

    if (frameId == 0x324 && (frameType == 't' || frameType == 'T')) {
        const QByteArray rawData = QByteArray::fromHex(payload);
        if (rawData.size() >= 2) {
            const quint8 charge = static_cast<quint8>(rawData.at(0));
            const quint8 discharge = static_cast<quint8>(rawData.at(1));
            appendLog(QStringLiteral("RX MOS id=0x324 charge=%1 discharge=%2")
                      .arg(QString::number(charge),
                           QString::number(discharge)));
            if (m_waitingMosAck
                && charge == m_pendingChargeMos
                && discharge == m_pendingDischargeMos) {
                m_waitingMosAck = false;
                appendLog(QStringLiteral("MOS回执成功：对端状态已返回"));
            }
        } else {
            appendLog(QStringLiteral("RX MOS id=0x324 数据长度不足: %1").arg(QString::number(rawData.size())));
        }
        return;
    }

    if ((frameId >= 0x326 && frameId <= 0x329) && (frameType == 't' || frameType == 'T')) {
        const QByteArray rawData = QByteArray::fromHex(payload);
        const auto readLeU16 = [&rawData](int offset, quint16 &value) -> bool {
            if (rawData.size() < offset + 2) {
                return false;
            }
            value = static_cast<quint8>(rawData.at(offset))
                    | (static_cast<quint16>(static_cast<quint8>(rawData.at(offset + 1))) << 8);
            return true;
        };
        const auto setCellLabel = [this](QLabel *label, quint16 mV) {
            label->setText(QString::number(static_cast<double>(mV) / 1000.0, 'f', 3) + QStringLiteral(" V"));
        };

        quint16 c1 = 0, c2 = 0, c3 = 0, c4 = 0;
        if (frameId == 0x326 && readLeU16(0, c1) && readLeU16(2, c2) && readLeU16(4, c3) && readLeU16(6, c4)) {
            setCellLabel(ui->label_25, c1);
            setCellLabel(ui->label_10, c2);
            setCellLabel(ui->label_11, c3);
            setCellLabel(ui->label_12, c4);
        } else if (frameId == 0x327 && readLeU16(0, c1) && readLeU16(2, c2) && readLeU16(4, c3) && readLeU16(6, c4)) {
            setCellLabel(ui->label_13, c1);
            setCellLabel(ui->label_26, c2);
            setCellLabel(ui->label_15, c3);
            setCellLabel(ui->label_16, c4);
        } else if (frameId == 0x328 && readLeU16(0, c1) && readLeU16(2, c2) && readLeU16(4, c3) && readLeU16(6, c4)) {
            setCellLabel(ui->label_17, c1);
            setCellLabel(ui->label_18, c2);
            setCellLabel(ui->label_27, c3);
            setCellLabel(ui->label_6, c4);
        } else if (frameId == 0x329 && readLeU16(0, c1) && readLeU16(2, c2) && readLeU16(4, c3)) {
            setCellLabel(ui->label_8, c1);
            setCellLabel(ui->label_7, c2);
            setCellLabel(ui->label_4, c3);
        } else {
            appendLog(QStringLiteral("RX id=0x%1 电芯电压帧长度不足，dlc=%2 data=%3")
                      .arg(QString::number(frameId, 16).toUpper(),
                           QString::number(dlc),
                           QString::fromLatin1(payload)));
            return;
        }

        appendLog(QStringLiteral("RX id=0x%1 电芯电压更新")
                  .arg(QString::number(frameId, 16).toUpper()));
        return;
    }

    if (frameId != 0x321) {
        return;
    }

    if (frameType == 't' || frameType == 'T') {
        const QByteArray rawData = QByteArray::fromHex(payload);
        if (rawData.size() >= 6) {
            const quint16 pack_mV = static_cast<quint8>(rawData.at(0))
                    | (static_cast<quint16>(static_cast<quint8>(rawData.at(1))) << 8);
            const quint16 currentRaw = static_cast<quint8>(rawData.at(2))
                    | (static_cast<quint16>(static_cast<quint8>(rawData.at(3))) << 8);
            const qint16 current_mA = static_cast<qint16>(currentRaw);
            const quint8 soc = static_cast<quint8>(rawData.at(4));

            const double pack_V = static_cast<double>(pack_mV) / 1000.0;
            const double current_A = static_cast<double>(current_mA) / 1000.0;

            ui->label_2->setText(QString::number(pack_V, 'f', 3) + QStringLiteral(" V"));
            ui->label_21->setText(QString::number(current_A, 'f', 3) + QStringLiteral(" A"));
            ui->label_23->setText(QString::number(soc) + QStringLiteral(" %"));
        }

        appendLog(QStringLiteral("RX id=0x321 dlc=%1 data=%2")
                  .arg(QString::number(dlc),
                       QString::fromLatin1(payload)));
    } else {
        appendLog(QStringLiteral("RX id=0x321 RTR dlc=%1").arg(QString::number(dlc)));
    }
}

void MainWindow::disconnectCan()
{
    appendLog(QStringLiteral("开始断开连接"));
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            sendSlcanCommand(QByteArrayLiteral("C\r"));
            m_serialPort->close();
        }
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
        m_slcanRxBuffer.clear();
        appendLog(QStringLiteral("串口CAN已断开"));
    }
    if (!m_canDevice) {
        setCanUiConnected(false);
        appendLog(QStringLiteral("断开完成"));
        return;
    }
    m_canDevice->disconnectDevice();
    m_canDevice->deleteLater();
    m_canDevice = nullptr;
    setCanUiConnected(false);
    appendLog(QStringLiteral("Qt CAN已断开"));
}

void MainWindow::on_dischargemos_open_2_clicked()
{
    on_can_connect_clicked();
}

void MainWindow::on_dischargemos_close_2_clicked()
{
    on_can_disconnect_clicked();
}

void MainWindow::on_can_connect_clicked()
{
    connectCan();
}

void MainWindow::on_can_disconnect_clicked()
{
    disconnectCan();
}

void MainWindow::on_dischargemos_open_clicked()
{
    m_dischargeMosState = 1;
    sendMosControlFrame(m_chargeMosState, m_dischargeMosState);
}

void MainWindow::on_dischargemos_close_clicked()
{
    m_dischargeMosState = 0;
    sendMosControlFrame(m_chargeMosState, m_dischargeMosState);
}

void MainWindow::on_chargemos_close_clicked()
{
    m_chargeMosState = 0;
    sendMosControlFrame(m_chargeMosState, m_dischargeMosState);
}

void MainWindow::on_chargemos_open_clicked()
{
    m_chargeMosState = 1;
    sendMosControlFrame(m_chargeMosState, m_dischargeMosState);
}

void MainWindow::on_comboBox_activated(const QString &)
{
    const QVariantMap data = ui->comboBox->currentData().toMap();
    g_canPlugin = data.value(QStringLiteral("plugin")).toString();
    g_canInterfaceName = data.value(QStringLiteral("interface")).toString();
    g_canConfig[QCanBusDevice::BitRateKey] = ui->comboBox_2->currentData();
    g_canConfig[QCanBusDevice::CanFdKey] = ui->comboBox_3->currentData();
}


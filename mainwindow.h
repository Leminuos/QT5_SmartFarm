#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "light_sensor.h"
#include "temperature_humidity_sensor.h"
#include "gpio.h"

#include <QTimer>
#include <QFile>
#include <QDebug>
#include "mqttclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    int init();

    void deinit();

private slots:
    void on_lightButton_clicked();

    void on_pumpButton_clicked();

private:
    Ui::MainWindow *ui;

    QTimer mReadSensorDataTimer;

    MqttClient client;

    LightSensor mLightSensor;

    TemperatureHumiditySensor mTemperatureHumiditySensor;

    Gpio mLightGpio;

    Gpio mPumpGpio;

    void onToggleLight(void);
    void onTogglePump(void);
};
#endif // MAINWINDOW_H

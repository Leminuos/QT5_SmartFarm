#include "mainwindow.h"
#include "ui_mainwindow.h"

#define LIGHT_SENSOR_DEVICE_PATH                "/dev/i2c-1"
#define LIGHT_SENSOR_DEVICE_ADDRESS             0x23

#define TEMPERATURE_HUMIDITY_DEVICE_PATH        "/dev/i2c-1"
#define TEMPERATURE_HUMIDITY_DEVICE_ADDRESS     0x44

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::init()
{
    int ret;

    ret = mLightSensor.init(LIGHT_SENSOR_DEVICE_PATH, LIGHT_SENSOR_DEVICE_ADDRESS);
    if (ret < 0)
    {
        printf("Failed to init light sensor\n");
        return -1;
    }

    ret = mTemperatureHumiditySensor.init(TEMPERATURE_HUMIDITY_DEVICE_PATH, TEMPERATURE_HUMIDITY_DEVICE_ADDRESS);
    if (ret < 0)
    {
        printf("Failed to init temperature humidity sensor\n");
        return -1;
    }

    ret = mLightGpio.init("light", "gpiochip2", 2); /* P8_7 pin */
    if (ret < 0)
    {
        printf("Failed to init light GPIO\n");
        return -1;
    } 
    else 
    {
        mLightGpio.setState(GPIO_LOW);
        ui->lightButton->setStyleSheet("background-color: #9E9E9E;");
    }

    ret = mPumpGpio.init("pump", "gpiochip2", 3); /* P8_8 pin */
    if (ret < 0)
    {
        printf("Failed to init pump GPIO\n");
        return -1;
    }
    else 
    {
        mPumpGpio.setState(GPIO_LOW);
        ui->pumpButton->setStyleSheet("background-color: #9E9E9E;");
    }

    QObject::connect(&client, &MqttClient::connected, [&]() {
        qDebug() << "Connected to MQTT broker";

        client.subscribeTopic("control/led");
        client.subscribeTopic("control/pump");

        int state = mLightGpio.getState();
        client.publishMessage(
            "state/led",
            QByteArray::number(state),
            0,
            true
        );

        state = mPumpGpio.getState();

        client.publishMessage(
            "state/pump",
            QByteArray::number(state),
            0,
            true
        );
    });

    QObject::connect(&client, &MqttClient::messageReceived, [&](const QString &topic, const QByteArray &payload) {
        if (topic == "control/led")
        {
            onToggleLight();
        }
        else if (topic == "control/pump")
        {
            onTogglePump();
        }
    });

    QObject::connect(&client, &MqttClient::errorOccurred, [](const QString &err) {
        qWarning() << "MQTT error:" << err;
    });

    QObject::connect(&mReadSensorDataTimer, &QTimer::timeout, [&]() {
        TemperatureHumiditySensor::CelsiusHumidityValue celsiusHumidityValue;
        int luxValue;

        luxValue = mLightSensor.readLuxValue();
        celsiusHumidityValue = mTemperatureHumiditySensor.readCelsiusHumidityValue();

        client.publishMessage(
            "sensor/temp",
            QByteArray::number((double)celsiusHumidityValue.temperature, 'f', 1),
            0,
            true
        );
        client.publishMessage(
            "sensor/humi",
            QByteArray::number((int)celsiusHumidityValue.humidity),
            0,
            true
        );
        client.publishMessage(
            "sensor/lux",
            QByteArray::number(luxValue),
            0,
            true
        );

        ui->tempValLabel->setText(QString("%1").arg((double)celsiusHumidityValue.temperature, 0, 'f', 1));
        ui->humidValLabel->setText(QString("%1").arg((int)celsiusHumidityValue.humidity));
        ui->lightValLabel->setText(QString("%1").arg(luxValue));
    });

    mReadSensorDataTimer.start(3000); // every 3000 ms

    client.connectToHost("127.0.0.1", 1883, 60);
    return 0;
}

void MainWindow::on_lightButton_clicked()
{
    onToggleLight();
}

void MainWindow::on_pumpButton_clicked()
{
    onTogglePump();
}

void MainWindow::onToggleLight(void)
{
    int state = mLightGpio.getState();

    if (state == GPIO_HIGH)
    {
        mLightGpio.setState(GPIO_LOW);
        ui->lightButton->setStyleSheet("background-color: #9E9E9E;");
        client.publishMessage(
            "state/led",
            "0",
            0,
            true
        );
    }
    else
    {
        mLightGpio.setState(GPIO_HIGH);
        ui->lightButton->setStyleSheet("background-color: #4CAF50;");
        client.publishMessage(
            "state/led",
            "1",
            0,
            true
        );
    }
}

void MainWindow::onTogglePump(void)
{
    int state = mPumpGpio.getState();

    if (state == GPIO_HIGH)
    {
        mPumpGpio.setState(GPIO_LOW);
        ui->pumpButton->setStyleSheet("background-color: #9E9E9E;");
        client.publishMessage(
            "state/pump",
            "0",
            0,
            true
        );
    }
    else
    {
        mPumpGpio.setState(GPIO_HIGH);
        ui->pumpButton->setStyleSheet("background-color: #4CAF50;");
        client.publishMessage(
            "state/pump",
            "1",
            0,
            true
        );
    }
}

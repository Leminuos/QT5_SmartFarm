#include "mqttclient.h"
#include <QDebug>

MqttClient::MqttClient(QObject *parent)
    : QObject(parent),
      m_mosq(nullptr),
      m_port(1883),
      m_keepalive(60)
{
    // Khởi tạo thư viện mosquitto
    mosquitto_lib_init();

    m_mosq = mosquitto_new("bbb-qt-client", true, this);
    if (!m_mosq)
    {
        emit errorOccurred("mosquitto_new() failed");
        return;
    }

    mosquitto_connect_callback_set(m_mosq, &MqttClient::on_connect);
    mosquitto_disconnect_callback_set(m_mosq, &MqttClient::on_disconnect);
    mosquitto_message_callback_set(m_mosq, &MqttClient::on_message);

    // Timer để gọi mosquitto_loop()
    connect(&m_loopTimer, &QTimer::timeout, this, &MqttClient::processLoop);
    m_loopTimer.start(10); // 10ms
}

MqttClient::~MqttClient()
{
    m_loopTimer.stop();

    if (m_mosq)
    {
        mosquitto_disconnect(m_mosq);
        mosquitto_destroy(m_mosq);
        m_mosq = nullptr;
    }

    mosquitto_lib_cleanup();
}

void MqttClient::connectToHost(const QString &host, int port, int keepalive)
{
    if (!m_mosq)
    {
        emit errorOccurred("MQTT client is not initialized");
        return;
    }

    m_host = host;
    m_port = port;
    m_keepalive = keepalive;

    int rc = mosquitto_connect_async(
        m_mosq,
        m_host.toUtf8().constData(),
        m_port,
        m_keepalive
    );

    if (rc != MOSQ_ERR_SUCCESS)
    {
        emit errorOccurred(QString("mosquitto_connect_async() failed: %1")
                           .arg(mosquitto_strerror(rc)));
    }
}

void MqttClient::disconnectFromHost()
{
    if (m_mosq)
    {
        mosquitto_disconnect(m_mosq);
    }
}

void MqttClient::publishMessage(const QString &topic,
                                const QByteArray &payload,
                                int qos,
                                bool retain)
{
    if (!m_mosq) return;

    int rc = mosquitto_publish(
        m_mosq,
        nullptr, // message id (null = không cần)
        topic.toUtf8().constData(),
        payload.size(),
        payload.constData(),
        qos,
        retain
    );

    if (rc != MOSQ_ERR_SUCCESS)
    {
        emit errorOccurred(QString("mosquitto_publish() failed: %1")
                            .arg(mosquitto_strerror(rc)));
    }
}

void MqttClient::subscribeTopic(const QString &topic, int qos)
{
    if (!m_mosq) return;

    int rc = mosquitto_subscribe(
        m_mosq,
        nullptr,
        topic.toUtf8().constData(),
        qos
    );

    if (rc != MOSQ_ERR_SUCCESS)
    {
        emit errorOccurred(QString("mosquitto_subscribe() failed: %1").arg(mosquitto_strerror(rc)));
    }
}

void MqttClient::processLoop()
{
    if (!m_mosq) return;

    int rc = mosquitto_loop(m_mosq, 0, 1);

    if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN)
    {
        emit errorOccurred(QString("mosquitto_loop() error: %1").arg(mosquitto_strerror(rc)));

        if (rc == MOSQ_ERR_CONN_LOST)
        {
            qDebug() << "MQTT connection lost, try reconnect...";
            mosquitto_reconnect_async(m_mosq);
        }
    }
}

void MqttClient::on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    Q_UNUSED(mosq);
    auto *self = static_cast<MqttClient*>(userdata);
    if (!self) return;

    if (rc == 0)
    {
        qDebug() << "MQTT connected";
        emit self->connected();
    }
    else
    {
        emit self->errorOccurred(QString("MQTT connect failed, rc=%1").arg(rc));
    }
}

void MqttClient::on_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    Q_UNUSED(mosq);
    auto *self = static_cast<MqttClient*>(userdata);
    if (!self) return;

    qDebug() << "MQTT disconnected, rc=" << rc;
    emit self->disconnected(rc);
}

void MqttClient::on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg)
{
    Q_UNUSED(mosq);
    auto *self = static_cast<MqttClient*>(userdata);
    if (!self || !msg) return;

    QString topic = QString::fromUtf8(msg->topic);
    QByteArray payload(static_cast<const char*>(msg->payload), msg->payloadlen);

    emit self->messageReceived(topic, payload);
}
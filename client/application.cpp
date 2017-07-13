#include "application.h"
#include "message.pb.h"

#include <QCoreApplication>
#include <QDebug>

namespace protobuf_client_example {

  namespace {
    const uint32_t MAGIC_NUMBER = 12345;

    void encode(char* ptr, uint32_t data) {
      ptr[0] = static_cast<char>((data & 0xFF000000) >> 24); 
      ptr[1] = static_cast<char>((data & 0x00FF0000) >> 16); 
      ptr[2] = static_cast<char>((data & 0x0000FF00) >> 8); 
      ptr[3] = static_cast<char>((data & 0x000000FF));
    }

    uint32_t decode(char* ptr) {
        uint32_t data = static_cast<uint32_t>(ptr[0]);
        data = data << 8;
        data += static_cast<uint32_t>(ptr[1]);
        data = data << 8;
        data += static_cast<uint32_t>(ptr[2]);
        data = data << 8;
        data += static_cast<uint32_t>(ptr[3]);
        return data;
    }
  }

  Application::Application() {
    sendBuffer.resize(1024);
    QObject::connect(&socket, &QTcpSocket::connected, this, &Application::onConnected);
    QObject::connect(&socket, &QTcpSocket::disconnected, this, &Application::onDisconnected);
    QObject::connect(&socket, &QTcpSocket::readyRead, this, &Application::onReadyRead);
  }

  void Application::start(const QString &url, int port) {
    qDebug() << "Application start";
    socket.connectToHost(url, port);
  }

  void Application::onConnected() {
    qDebug() << "Socket connected sending first message";
    protocol::Message message;
    message.set_type(protocol::Message_Type_GET_EVENTS_REQUEST);
    protocol::GetEventRequest* request =  message.mutable_geteventrequest();
    send(message);
  }

  void Application::onDisconnected() {
    qDebug() << "Socket disconnected, closing the application";
    qApp->quit();
  }

  void Application::onReadyRead()
  {
      QByteArray data = socket.readAll();
      if (data.size() < 4)
          return;
      uint32_t magic_number = decode(data.data());
      if (data.size() < 8)
          return;
      uint32_t message_size = decode(data.data() + 4);
      if (data.size() < (8 + message_size))
          return;
      protocol::Message message;
      message.ParseFromArray(data.data() + 8, message_size);
      onMessageReceived(message);
  }

  void Application::send(const protocol::Message &message) {
    const int message_size = 2 * sizeof(uint32_t) + message.ByteSize();
    sendBuffer.resize(message_size);
    encode(&sendBuffer[0], MAGIC_NUMBER);
    encode(&sendBuffer[4], message.ByteSize());
    const bool ok = message.SerializeToArray(&sendBuffer[8], sendBuffer.size() - 8);
    if (ok) {
      qDebug() << "Sending a message through the socket";
      socket.write(sendBuffer.data(), sendBuffer.size());
    }
  }

  void Application::onMessageReceived(const protocol::Message &message)
  {
      switch (message.type())
      {
      case protocol::Message_Type_SET_EVENTS_REPLY:
          qDebug() << "GetEventsReply received";
          break;
      default:
          qDebug() << "Unknown message received";
      }
  }
}

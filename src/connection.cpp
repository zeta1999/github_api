
#include "connection.hpp"

#include <cassert>
#include <string>

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QSignalMapper>


namespace GitHub
{

    Connection::Connection(QNetworkAccessManager& manager, const QString& address, const QString& token):
        m_networkManager(manager),
        m_signalMapper(new QSignalMapper(this)),
        m_address(address),
        m_token(token),
        m_replys()
    {
        connect(m_signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(gotReply(QObject *)));
    }


    Connection::~Connection()
    {

    }


    QNetworkRequest Connection::prepareRequest()
    {
        QNetworkRequest requst;

        if (m_token.isEmpty() == false)
        {
            const QByteArray key("Authorization");
            const QByteArray value = QString("token %1").arg(m_token).toLatin1();
            requst.setRawHeader(key, value);
        }

        return requst;
    }


    void Connection::get(const QString& query, const std::function<void(const QJsonDocument &)>& callback)
    {
        QNetworkRequest request = prepareRequest();
        const QUrl url = QString("%1/%2").arg(m_address).arg(query);
        request.setUrl(url);

        QNetworkReply* reply = m_networkManager.get(request);

        m_replys[reply] = callback;
        m_signalMapper->setMapping(reply, reply);
        connect(reply, SIGNAL(readChannelFinished()), m_signalMapper, SLOT(map()));
    }


    void Connection::gotReply(QObject* reply_obj)
    {
        auto callback_it = m_replys.find(reply_obj);
        assert(callback_it != m_replys.end());

        if (callback_it != m_replys.end())
        {
            auto callback = callback_it->second;
            assert(dynamic_cast<QNetworkReply *>(reply_obj));
            QNetworkReply* reply = static_cast<QNetworkReply *>(reply_obj);

            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(data);

            callback(doc);
        }

        reply_obj->deleteLater();
        reply_obj = nullptr;
    }

}

// std lib
#include <csignal>
#include <memory>
// Qt
#include <QByteArray>
#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QtConcurrent>
#include <QThread>
//
#include "MutexedPool.h"
#include "ThreadedDatabase.h"

using namespace std;

void signalHandler(int signum)
{
    qCritical() << "[INFO] catching signal: " << signum;
    QCoreApplication::quit();
}

QJsonObject byteArrayToJsonObject(const QByteArray &arr)
{
    const auto json = QJsonDocument::fromJson(arr);
    return json.object();
}

bool transactionIsValid(const QJsonObject &body)
{
    if (body.value("valor").toInt(-1) < 0 || body.value("descricao").isNull() || body.value("tipo").isNull())
        return false;
    QString descricao = body.value("descricao").toString();
    return body.value("valor").toInt() > 0 &&
           !descricao.isEmpty() &&
           descricao.size() < 11 &&
           QStringList{"d", "c"}.contains(body.value("tipo").toString());
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QHttpServer httpServer;
    MutexedPool<ThreadedDatabase> dbPool;
    std::vector<shared_ptr<QThread>> threads;
    for (auto i = 0; i < 10; ++i)
    {
        auto db = make_shared<ThreadedDatabase>(i);
        auto thread = make_shared<QThread>(nullptr);
        db->moveToThread(thread.get());
        thread->start();
        threads.push_back(thread);
        dbPool.push(std::move(db));
    }
    std::vector<int> validClients = {1, 2, 3, 4, 5};

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    const auto port = httpServer.listen(QHostAddress::Any, 3000);

    if (!port)
        return 0;

    httpServer.route("/clientes/<arg>/extrato",
                     QHttpServerRequest::Method::Get,
                     [&dbPool, validClients](int clientId)
                     {
                         return QtConcurrent::run([&dbPool, validClients, clientId]()
                                                  {
                            if (std::find(validClients.begin(),
                                        validClients.end(),
                                        clientId) != validClients.end())
                            {
                                shared_ptr<ThreadedDatabase> db = dbPool.pop();
                                QJsonObject saldo = db->extrato(clientId);
                                dbPool.push(std::move(db));
                                return QHttpServerResponse(saldo, QHttpServerResponder::StatusCode::Ok);
                            }
                            return QHttpServerResponse("error", QHttpServerResponder::StatusCode::NotFound); });
                     });

    httpServer.route("/clientes/<arg>/transacoes",
                     QHttpServerRequest::Method::Post,
                     [&dbPool, validClients](int clientId, const QHttpServerRequest &request)
                     {
                         return QtConcurrent::run([&dbPool, validClients, clientId, &request]()
                                                  {
                         if (std::find(validClients.begin(), validClients.end(), clientId) != validClients.end())
                         {
                             const QJsonObject newObject = byteArrayToJsonObject(request.body());
                             if (!transactionIsValid(newObject))
                             {
                                 return QHttpServerResponse("Invalid json request!", QHttpServerResponder::StatusCode::UnprocessableEntity);
                             }
                             shared_ptr<ThreadedDatabase> db = dbPool.pop();
                             QHttpServerResponder::StatusCode responseCode;
                             auto responseBody = db->transacao(clientId, newObject, responseCode);
                             dbPool.push(std::move(db));
                             return QHttpServerResponse(responseBody, responseCode);
                         }
                         return QHttpServerResponse("error", QHttpServerResponder::StatusCode::NotFound); });
                     });

    qCritical() << QString("QHttpServerExample - Running on http://127.0.0.1:%1/ (Press CTRL+C to quit)").arg(port);
    auto app_status = app.exec();
    qCritical() << "Terminating: " << app_status;
    for (auto &t : threads)
    {
        t->quit();
        t->wait(100);
    }
    threads.clear();
    return app_status;
}
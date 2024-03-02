#include "ThreadedDatabase.h"

#include <QJsonArray>
#include <QSqlError>

ThreadedDatabase::ThreadedDatabase(int oid, QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QPSQL", QString("pgsql-conn-%1").arg(oid));
    db.setConnectOptions("requiressl=0");
    db.setHostName("postgres");
    db.setDatabaseName("postgres");
    db.setUserName("postgres");
    db.setPassword("postgres");
    db.setPort(5432);
    auto opened = db.open();
    if (!opened) {
        qFatal("Terminate. No connection to database!");
    }
    qCritical() << "[INFO]" << Q_FUNC_INFO << "isOpened:" << opened;
    balance_query.reset(new QSqlQuery(db));
    auto balance_query_prepared = balance_query->prepare("SELECT limit_amount,balance \
                            FROM accounts \
                            WHERE id = :clientId");
    last_transactions.reset(new QSqlQuery(db));
    auto last_transactions_prepared = last_transactions->prepare("SELECT \
                                    amount, \
                                    transaction_type, \
                                    description, \
                                    TO_CHAR(date, 'YYYY-MM-DD HH:MI:SS.US') AS date \
                                FROM transactions \
                                WHERE account_id = :clientId \
                                ORDER BY date DESC \
                                LIMIT 10");
    for_update_select.reset(new QSqlQuery(db));
    auto for_update_select_prepared = for_update_select->prepare("SELECT limit_amount,balance \
                                    FROM accounts \
                                    WHERE id = :clientId \
                                    FOR UPDATE");

    insertQuery.reset(new QSqlQuery(db));
    auto insertQueryPrepared = insertQuery->prepare("INSERT INTO transactions \
                            (account_id, amount, transaction_type, description) \
                            VALUES (:clientId, :valor, :tipo, :descricao)");
    updateQuery.reset(new QSqlQuery(db));
    auto updateQueryPrepared = updateQuery->prepare("UPDATE accounts \
                            SET balance = balance + :transactionAmount \
                            WHERE accounts.id = :clientId");
    qCritical() << "[INFO]" << Q_FUNC_INFO
                << "balance_query_prepared" << balance_query_prepared
                << "last_transactions_prepared" << last_transactions_prepared
                << "for_update_select_prepared" << for_update_select_prepared
                << "insertQueryPrepared" << insertQueryPrepared
                << "updateQueryPrepared" << updateQueryPrepared;
}

QJsonObject ThreadedDatabase::extrato(int clientId)
{
    auto transactionStatus = db.transaction();
    // qCritical() << "[INFO]" << Q_FUNC_INFO << "transactionStatus" << transactionStatus << "last error" << db.lastError();
    balance_query->bindValue(":clientId", clientId);
    auto execStatus = balance_query->exec();
    auto nextStatus = balance_query->next();
    // qCritical() << "[INFO]" << Q_FUNC_INFO << "exec" << execStatus;
    // qCritical() << "[INFO]" << Q_FUNC_INFO << "query lasterror" << balance_query->lastError();
    int limit_amount = balance_query->value(0).toInt();
    int balance = balance_query->value(1).toInt();
    QJsonObject saldo;
    QJsonObject inner_saldo;
    inner_saldo["total"] = balance;
    inner_saldo["data_extrato"] = QDateTime::currentDateTime().toUTC().toString(Qt::ISODateWithMs);
    inner_saldo["limite"] = limit_amount;
    saldo["saldo"] = QJsonValue(inner_saldo);
    QJsonArray transactions;
    last_transactions->bindValue(":clientId", clientId);
    last_transactions->exec();
    while (last_transactions->next())
    {
        transactions.push_back(
            transaction(last_transactions->value(0).toInt(),
                        last_transactions->value(1).toString(),
                        last_transactions->value(2).toString(),
                        last_transactions->value(3).toString()));
    }
    saldo["ultimas_transacoes"] = QJsonValue(transactions);
    db.commit();
    return saldo;
}

QJsonObject ThreadedDatabase::transacao(int clientId, const QJsonObject &newTransaction, QHttpServerResponder::StatusCode &responseCode)
{
    auto transactionStatus = db.transaction();
    // qCritical() << "[INFO]" << Q_FUNC_INFO << "transactionStatus" << transactionStatus << "last error" << db.lastError();
    QJsonObject responseBody;
    for_update_select->bindValue(":clientId", clientId);
    for_update_select->exec();
    for_update_select->next();
    int limitAmount = for_update_select->value(0).toInt();
    int balance = for_update_select->value(1).toInt();
    int transactionAmount = newTransaction.value("valor").toInt();
    QString transactionType = newTransaction.value("tipo").toString();
    QString transactionDescription = newTransaction.value("descricao").toString();
    if (transactionType == "d" && isTransactionBrokeLimit(balance, limitAmount, transactionAmount))
    {
        responseBody.insert("error", "Client has no available limit to execute the transaction");
        responseCode = QHttpServerResponder::StatusCode::UnprocessableEntity;
    }
    else
    {
        insertQuery->bindValue(":clientId", clientId);
        insertQuery->bindValue(":valor", transactionAmount);
        insertQuery->bindValue(":tipo", transactionType);
        insertQuery->bindValue(":descricao", transactionDescription);
        insertQuery->exec();
        //  insertQuery.finish();
        if (transactionType == "c")
        {
            updateQuery->bindValue(":transactionAmount", transactionAmount);
            balance += transactionAmount;
        }
        else
        {
            updateQuery->bindValue(":transactionAmount", -transactionAmount);
            balance -= transactionAmount;
        }
        updateQuery->bindValue(":clientId", clientId);
        updateQuery->exec();
        //  updateQuery.finish();
        responseCode = QHttpServerResponder::StatusCode::Ok;
        responseBody.insert("limite", limitAmount);
        responseBody.insert("saldo", balance);
    }
    db.commit();
    return responseBody;
}

QJsonValue ThreadedDatabase::transaction(int amount, const QString &transaction_type, const QString &description, const QString &transaction_date)
{
    QJsonObject obj;
    obj.insert("valor", QJsonValue(amount));
    obj.insert("tipo", QJsonValue(transaction_type));
    obj.insert("descricao", QJsonValue(description));
    obj.insert("realizada_em", QJsonValue(transaction_date));
    return QJsonValue(obj);
}

bool ThreadedDatabase::isTransactionBrokeLimit(int balance, int limit, int transaction_amount)
{
    return ((balance - transaction_amount) < -limit);
}
#include <memory>

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QThread>
#include <QHttpServerResponse>

class ThreadedDatabase : public QObject
{
    Q_OBJECT
    QSqlDatabase db;
    std::unique_ptr<QSqlQuery> balance_query;
    std::unique_ptr<QSqlQuery> last_transactions;
    std::unique_ptr<QSqlQuery> for_update_select;
    std::unique_ptr<QSqlQuery> insertQuery;
    std::unique_ptr<QSqlQuery> updateQuery;

public:
    explicit ThreadedDatabase(int oid, QObject *parent = nullptr);
    /**
     * @brief Retorna as últimas 10 transacoes do cliente
     *
     * @param clientId
     * @return QJsonObject
     */
    QJsonObject extrato(int clientId);
    /**
     * @brief Tenta executar uma transacao (credito/debito). Caso haja limite
     * suficiente, insere transacao no banco de dados, atualiza saldo do
     * cliente e retorna resposta válida responseCode=200::Ok.
     * Caso não haja limite suficiente, retorna
     * responseCode=422::UnprocessableEntity e devolve um json com saldo atual,
     * limite, valor da transacao e string de erro.
     *
     * @param clientId
     * @param newTransaction
     * @param responseCode - valor que será preenchido
     * (200::Ok para transacoes válidas)
     * (422::UnprocessableEntity para transacoes inválidas)
     * @return QJsonObject - retorna informacoes exigidas pela API ou mensagem
     * de erro.
     */
    QJsonObject transacao(int clientId, const QJsonObject &newTransaction, QHttpServerResponder::StatusCode &responseCode);

protected:
    /**
     * @brief Monta json utilizado para reportar as últimas 10 transacoes.
     *
     * @param amount
     * @param transaction_type
     * @param description
     * @param transaction_date
     * @return QJsonValue
     */
    QJsonValue transaction(int amount, const QString &transaction_type, const QString &description, const QString &transaction_date);
    /**
     * @brief Checa se a transacao irá extrapolar o limite do cliente.
     *
     * @param balance
     * @param limit
     * @param transaction_amount
     * @return true
     * @return false
     */
    bool isTransactionBrokeLimit(int balance, int limit, int transaction_amount);
};

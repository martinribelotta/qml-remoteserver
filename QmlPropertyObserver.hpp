#ifndef QMLPROPERTYOBSERVER_HPP
#define QMLPROPERTYOBSERVER_HPP

#include <QObject>
#include <QQmlProperty>
#include <QMetaProperty>
#include <functional>
#include <memory>

class QmlPropertyObserver : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(QVariant newValue)>;

    static QmlPropertyObserver* watch(const QQmlProperty& qmlProp, Callback cb, QObject* parent = nullptr) {
        QObject* object = qmlProp.object();
        const QString propName = qmlProp.name();

        if (!object || propName.isEmpty()) {
            qDebug() << "QmlPropertyObserver: Invalid object or property name.";
            return nullptr;
        }

        const QMetaObject* meta = object->metaObject();
        int index = meta->indexOfProperty(propName.toUtf8().constData());
        if (index == -1) {
            qDebug() << "QmlPropertyObserver: Property not found in object meta-object.";
            return nullptr;
        }

        QMetaProperty prop = meta->property(index);
        if (!prop.hasNotifySignal()) {
            qDebug() << "QmlPropertyObserver: Property does not have a notify signal.";
            return nullptr;
        }

        QmlPropertyObserver* observer = new QmlPropertyObserver(object, propName, std::move(cb), parent);

        QMetaMethod notifySignal = prop.notifySignal();
        QMetaMethod targetSlot = observer->metaObject()->method(observer->metaObject()->indexOfSlot("onPropertyChanged()"));

        // Store the connection so it can be managed later if needed
        observer->m_connection = QObject::connect(object, notifySignal, observer, targetSlot);
        return observer;
    }

    // Expose the connection for external management if needed
    QMetaObject::Connection connection() const { return m_connection; }

private:
    QmlPropertyObserver(QObject* obj, QString propName, Callback cb, QObject* parent)
        : QObject(parent), m_object(obj), m_propName(std::move(propName)), m_callback(std::move(cb)) {}

private slots:
    void onPropertyChanged() {
        QVariant val = QQmlProperty::read(m_object, m_propName);
        if (m_callback) m_callback(val);
    }

private:
    QObject* m_object;
    QString m_propName;
    Callback m_callback;
    QMetaObject::Connection m_connection;
};

#endif // QMLPROPERTYOBSERVER_HPP

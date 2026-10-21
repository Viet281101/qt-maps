#include <QDebug>
#include <QGuiApplication>
#include <QGeoServiceProvider>
#include <QQmlApplicationEngine>
#include <QStringList>
#include <QVariantMap>

namespace {

void probeProvider(const QString &providerName)
{
    QGeoServiceProvider provider(providerName, QVariantMap(), true);
    const auto *mappingManager = provider.mappingManager();

    qInfo().noquote()
        << QStringLiteral("provider=%1 constructed=true mappingManager=%2 error=%3 errorString=%4 mappingError=%5 mappingErrorString=%6")
               .arg(providerName,
                    mappingManager != nullptr ? QStringLiteral("loaded") : QStringLiteral("null"),
                    QString::number(static_cast<int>(provider.error())),
                    provider.errorString().isEmpty() ? QStringLiteral("<empty>") : provider.errorString(),
                    QString::number(static_cast<int>(provider.mappingError())),
                    provider.mappingErrorString().isEmpty()
                        ? QStringLiteral("<empty>")
                        : provider.mappingErrorString());
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    const QStringList providers = QGeoServiceProvider::availableServiceProviders();
    qInfo().noquote() << "availableServiceProviders=" << providers.join(QLatin1Char(','));
    probeProvider(QStringLiteral("osm"));
    probeProvider(QStringLiteral("mapboxgl"));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load qrc:/qml/Main.qml";
        return 1;
    }

    return application.exec();
}

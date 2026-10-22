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
    const bool useMapboxGlProbe = application.arguments().contains(QStringLiteral("--mapboxgl-probe"));

    const QStringList providers = QGeoServiceProvider::availableServiceProviders();
    qInfo().noquote() << "availableServiceProviders=" << providers.join(QLatin1Char(','));
    probeProvider(QStringLiteral("osm"));
    probeProvider(QStringLiteral("mapboxgl"));

    QQmlApplicationEngine engine;
    const QUrl qmlUrl(useMapboxGlProbe
                          ? QStringLiteral("qrc:/qml/MapboxGlProbe.qml")
                          : QStringLiteral("qrc:/qml/Main.qml"));
    qInfo().noquote() << "probeMode=" << (useMapboxGlProbe ? "mapboxgl-style-only" : "osm-diagnostic-raster");
    engine.load(qmlUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load" << qmlUrl;
        return 1;
    }

    return application.exec();
}

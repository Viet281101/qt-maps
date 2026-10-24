#pragma once

#include "Route.h"

#include <QByteArray>
#include <QString>

namespace qtmaps {
namespace routing {

struct RouteParseResult
{
    Route route;
    QString error;

    bool ok() const { return error.isEmpty() && route.isValid(); }
};

class ValhallaRouteParser final
{
public:
    static RouteParseResult parse(const QByteArray &response,
                                  RouteSource source = RouteSource::Valhalla);
};

} // namespace routing
} // namespace qtmaps

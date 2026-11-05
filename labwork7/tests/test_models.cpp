#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "models.hpp"

using namespace testing;
using json = nlohmann::json;

TEST(ModelsTest, SerializationAndDeserialization) {
    Station station;
    station.code = "c1";
    station.title = "Title";
    station.popular_title = "PopTitle";
    station.short_title = "Short";
    station.transport_type = "train";
    station.type = "station";

    json j_station = station;
    auto s2 = j_station.get<Station>();
    EXPECT_EQ(s2.code, "c1");

    Price price;
    price.cents = 50;
    price.whole = 100;
    price.currency = "RUB";
    EXPECT_FLOAT_EQ(price.GetTotal(), 100.5f);
    
    json j_price = price;
    auto p2 = j_price.get<Price>();
    EXPECT_EQ(p2.cents, 50);

    Place place;
    place.name = "Seat";
    place.price = price;
    place.currency = "RUB";

    json j_place = place;
    auto pl2 = j_place.get<Place>();
    EXPECT_EQ(pl2.name, "Seat");

    TicketsInfo ti;
    ti.et_marker = true;
    ti.places.push_back(place);
    
    Place p3;
    p3.name = "Cheaper";
    p3.price.cents = 0;
    p3.price.whole = 50;
    p3.price.currency = "RUB";
    p3.currency = "RUB";
    ti.places.push_back(p3);
    EXPECT_FLOAT_EQ(ti.GetMinPrice(), 50.0f);
    
    TicketsInfo ti_empty;
    ti_empty.et_marker = false;
    EXPECT_FLOAT_EQ(ti_empty.GetMinPrice(), 0.0f);
    
    json j_ti = ti;
    auto ti2 = j_ti.get<TicketsInfo>();
    EXPECT_TRUE(ti2.et_marker);

    Carrier carrier;
    carrier.code = 1;
    carrier.title = "C";
    carrier.url = "url";
    carrier.phone = "phone";
    carrier.contacts = "contacts";
    carrier.logo = "logo";

    Thread thread;
    thread.uid = "uid";
    thread.title = "T";
    thread.number = "num";
    thread.short_title = "short";
    thread.transport_type = "train";
    thread.vehicle = "veh";
    thread.carrier = carrier;

    Segment seg;
    seg.from = station;
    seg.to = station;
    seg.duration = 3600;
    seg.has_tickets_info = true;
    seg.tickets_info = ti;
    EXPECT_FLOAT_EQ(seg.GetPrice(), 50.0f);

    Route route;
    route.segments.push_back(seg);
    route.segments.push_back(seg);
    route.CalculateSummary();
    EXPECT_EQ(route.transfers_count, 1);
    EXPECT_EQ(route.total_duration, 7200);
}
#include "platform/battery.hpp"
#include <cassert>
#include <iostream>
using platform::Battery;
using platform::BatteryReading;
struct Fake { BatteryReading value; int reads=0; };
BatteryReading read(void *p) { auto &f=*static_cast<Fake*>(p);++f.reads;return f.value; }
int main() {
    Fake f{{true,5,false}}; Battery b;b.setSource(read,&f);
    b.update(0);assert(f.reads==1&&b.reading().level==5&&!b.warning());
    for(uint64_t t=1;t<30000;++t)b.update(t);
    assert(f.reads==1);
    f.value={true,1,false};b.update(30000);assert(f.reads==2&&b.warning());
    b.update(35999);assert(b.warning());b.update(36000);assert(!b.warning());
    b.update(60000);assert(!b.warning());
    f.value={};b.update(90000);assert(!b.reading().available&&!b.warning());
    f.value={true,1,false};b.update(120000);assert(!b.warning());
    f.value={true,2,false};b.update(150000);assert(!b.warning());
    f.value={true,1,false};b.update(180000);assert(!b.warning()); // hysteresis
    f.value={true,0,false};b.update(210000);assert(b.warning()); // real deterioration
    b.update(240000);assert(!b.warning());
    f.value={true,0,true};b.update(270000);assert(!b.warning()&&b.reading().charging);
    f.value={true,1,false};b.update(300000);assert(b.warning());
    f.value={true,3,false};b.update(330000);assert(!b.warning());
    f.value={true,1,false};b.update(360000);assert(b.warning());
    b.update(1);assert(!b.warning());b.update(30001);assert(!b.warning()); // clock reset no spam
    f.value={true,6,false};b.update(60001);assert(!b.reading().available);
    f.value={true,1,false};b.update(90001);assert(!b.warning());
    Battery host;host.update(0);assert(!host.reading().available&&!host.warning());
    f.value={true,0,false};b.setSource(read,&f);b.update(0);assert(b.warning());
    b.update(UINT64_MAX);assert(!b.warning());
    std::cout<<"Battery: coarse levels, charging, unavailable, polling, bounded alerts, hysteresis and clock reset passed\n";
}

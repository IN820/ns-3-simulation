#ifndef TRAFFICGEN_H
#define TRAFFICGEN_H
#include <string>
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/pointer.h"
#include "ns3/callback.h"
#include "ns3/random-variable-stream.h"
#include <vector>

// Usage Example Client app: traffic generator
// Ptr<TrafficGen> p_tgen = CreateObject<TrafficGen>();
// p_tgen -> SetAttribute("PacketSize", UintegerValue(gPacketSize));
// p_tgen -> SetAttribute("Remote", AddressValue(InetSocketAddress(sink_addr,8080))); // sink_addr is the IP address of the receiving node
// p_tgen -> SetAttribute("RemoteMAC", AddressValue(sink_MAC)); // sink_MAC is the MAC addr of the receiving node
//
//
// p_tgen -> SetStartTime(Seconds (gStartTime));
// p_tgen -> SetStopTime(Seconds (gEndTime));
//
// p_tgen -> SetAttribute("DataRate", DataRateValue(gMeanRate)); // the average traffic rate, takes a std::string
// p_tgen -> SetAttribute("IntervalType", StringValue("poisson"));
// source -> AddApplication(p_tgen);



namespace ns3{
  class TypeId;
  class Address;
  class DataRate;
  class Socket;
  class Time;
  class EventId;

  class TrafficGen : public Application
  {
 public:
    static TypeId GetTypeId();
    TrafficGen();
    ~TrafficGen();
    void SetDataRate(DataRate rate);
    void SetPacketSize(uint32_t pktsize);
    void SetRandomizer(int x);
    void SetRemote(std::string socketType, Address remote);
    int GetTotalTx();
    std::vector<double> GetInterpacket();

 private:
    virtual void StartApplication();
    virtual void StopApplication();

    void CancelEvents();
    void ScheduleNextTx();
    void SendPacket();
    void ConnectionSucceeded(ns3::Ptr<ns3::Socket>);
    void ConnectionFailed(ns3::Ptr<ns3::Socket>);

    TypeId m_tidProtocol;
    Ptr<Socket> m_socket;
    Ptr<ExponentialRandomVariable> p_rngExpo;
    Address m_peer;
    Address m_peerMAC;
    bool m_connected;
    int m_totalTx;
    DataRate m_rate;
    Time m_lastTxTime;
    uint32_t m_pktSize;
    std::string m_intervalType;
    EventId m_eventNextTx;
    TracedCallback<Ptr<const Packet> > m_txTrace;
    std::vector<double> m_interpacket_times;

  };
}

#endif

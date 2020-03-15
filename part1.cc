

// Network topology
//
//       n0 ----------- n1
//            1 Mbps
//             10 ms
//
// - Flow from n0 to n1 using BulkSendApplication.



// This code heavily borrows from ns3 itself which are copyright of their
// respective authors and redistributable under the same conditions.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("cbrComparision");

AsciiTraceHelper ascii;
Ptr<PacketSink> cbrSinks;

int totalVal;
int total_drops=0;
bool first_drop=true;

// Function to record packet drops
static void
RxDrop (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
    if(first_drop)
    {
        first_drop=false;
        *stream->GetStream ()<<0<<" "<<0<<std::endl;
    }
    *stream->GetStream ()<<Simulator::Now ().GetSeconds ()<<" "<<++total_drops<<std::endl;
}


// Function to find the total cumulative recieved bytes
static void
TotalRx(Ptr<OutputStreamWrapper> stream)
{

    
        totalVal = cbrSinks->GetTotalRx();
    

    *stream->GetStream()<<Simulator::Now ().GetSeconds ()<<" " <<totalVal<<std::endl;

    Simulator::Schedule(Seconds(0.0001),&TotalRx, stream);
}


int
main (int argc, char *argv[])
{

    bool tracing = false;
    uint32_t maxBytes = 0;
    std::string prot = "cbrModel";

    // Allow the user to override any of the defaults at
    // run-time, via command-line arguments
    CommandLine cmd;
    cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue ("maxBytes",
                  "Total number of bytes for application to send", maxBytes);

    cmd.Parse (argc, argv);
    
    std::string a_s = "bytes_"+prot+".dat";
    std::string b_s = "drop_"+prot+".dat";

    // Create file streams for data storage
    Ptr<OutputStreamWrapper> total_bytes_data = ascii.CreateFileStream (a_s);
    Ptr<OutputStreamWrapper> dropped_packets_data = ascii.CreateFileStream (b_s);

    // Explicitly create the nodes required by the topology (shown above).
    NS_LOG_INFO ("Create nodes.");
    NodeContainer nodes;
    nodes.Create (2);

    NS_LOG_INFO ("Create channels.");

    // Explicitly create the point-to-point link required by the topology (shown above).
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
    pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("50p"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    // Install the internet stack on the nodes
    InternetStackHelper internet;
    internet.Install (nodes);

    // We've got the "hardware" in place.  Now we need to add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Container = ipv4.Assign (devices);

    NS_LOG_INFO ("Create Applications.");

    // Create a BulkSendApplication and install it on node 0


    TrafficControlHelper tc;
    tc.Uninstall (devices);


    uint16_t cbrPort = 12345;
   
    // Install applications: five CBR streams each saturating the channel
    ApplicationContainer cbrApps;
    ApplicationContainer cbrSinkApps;
    OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (ipv4Container.GetAddress (1), cbrPort));
    onOffHelper.SetAttribute ("PacketSize", UintegerValue (1000));
    onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

    onOffHelper.SetAttribute ("DataRate", StringValue ("0.5Mbps"));
    onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.0)));
    onOffHelper.SetAttribute ("StopTime", TimeValue (Seconds (5.0)));
    cbrApps.Add (onOffHelper.Install (nodes.Get (0)));
    // Packet sinks for each CBR agent

    PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
    cbrSinkApps = sink.Install (nodes.Get (1));       
    cbrSinkApps.Start (Seconds (0.0));
    cbrSinkApps.Stop (Seconds (5.0));
    cbrSinks = DynamicCast<PacketSink> (cbrSinkApps.Get (0));
   

    devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, dropped_packets_data));

    // Enable tracing
    if (tracing)
    {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("cbr-comparision.tr"));
        pointToPoint.EnablePcapAll ("cbr-comparision", true);
    }

    NS_LOG_INFO ("Run Simulation.");

    Simulator::Schedule(Seconds(0.00001),&TotalRx, total_bytes_data);

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop (Seconds (5.00));
    Simulator::Run ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
    std::cout << std::endl << "***** Flow monitor *****" << std::endl;
    std::cout << "  Tx Packets:   " << stats[1].txPackets << std::endl;
    std::cout << "  Tx Bytes:   " << stats[1].txBytes << std::endl;
    std::cout << "  generated Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Rx Packets:   " << stats[1].rxPackets << std::endl;
    std::cout << "  Rx Bytes:   " << stats[1].rxBytes<< std::endl;
    std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Error Rate: " << ( stats[1].txPackets - stats[1].rxPackets ) * 100 / stats[1].txPackets << " % " << std::endl;
    flowMonitor->SerializeToXmlFile("data.flowmon", true, true);
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");

}

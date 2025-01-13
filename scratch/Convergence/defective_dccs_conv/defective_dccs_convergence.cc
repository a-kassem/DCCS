#include <iostream>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"


double flow1_previous_arrival = 0;
double flow2_previous_arrival = 0.2;
uint64_t flow1_previous_size = 0;
uint64_t flow2_previous_size = 0;
uint64_t count1 = 0;
uint64_t count2 = 0;

using namespace ns3;

std::ofstream F1cwnd;
std::ofstream F2cwnd;
std::ofstream F1Throughput;
std::ofstream F2Throughput;
std::ofstream F1rtt;
std::ofstream F2rtt;
std::ofstream t1QueueLength;
uint64_t F1Bytes = 0;
uint64_t F2Bytes = 0;

void
PrintProgress (Time interval)
{
    std::cout << "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << " seconds simulation time" << std::endl;
    Simulator::Schedule (interval, &PrintProgress, interval);
}

void
TraceF1Sink (Ptr<const Packet> p, const Address& a)
{
    F1Bytes += p->GetSize ();

    count1++;
    if(count1 == 1000)
    {
        count1 = 0;
        F1Throughput << Simulator::Now ().GetSeconds () << " :  Throughput     " << 8 * (F1Bytes - flow1_previous_size) << " / " << (Simulator::Now ().GetSeconds () - flow1_previous_arrival) << " = " << 8 * (F1Bytes - flow1_previous_size)/(Simulator::Now ().GetSeconds () - flow1_previous_arrival) << std::endl;
        flow1_previous_arrival = Simulator::Now ().GetSeconds ();
        flow1_previous_size = F1Bytes;
    }
}

void
TraceF2Sink ( Ptr<const Packet> p, const Address& a)
{
    F2Bytes += p->GetSize ();
    count2++;
    if(count2 == 1000)
    {
        count2 = 0;
        F2Throughput << Simulator::Now ().GetSeconds () << " :  Throughput     " << 8 * (F2Bytes - flow2_previous_size) << " / " << (Simulator::Now ().GetSeconds () - flow2_previous_arrival) << " = " << 8 * (F2Bytes - flow2_previous_size)/(Simulator::Now ().GetSeconds () - flow2_previous_arrival) << std::endl;
        flow2_previous_arrival = Simulator::Now ().GetSeconds ();
        flow2_previous_size = F2Bytes;
    }
}

void QueueLengthTracet1(uint32_t oldValue, uint32_t newValue)
{
    t1QueueLength << Simulator::Now().GetSeconds() << "\t" << newValue << std::endl;
}

void Cwnd1Change(uint32_t oldCwnd, uint32_t newCwnd) {
    F1cwnd << Simulator::Now().GetSeconds() << "\t" << round(newCwnd/1448) << std::endl;
}
void Cwnd2Change(uint32_t oldCwnd, uint32_t newCwnd) {
    F2cwnd << Simulator::Now().GetSeconds() << "\t" << round(newCwnd/1448) << std::endl;
}

void RTT1Change(Time oldRtt, Time newRtt) {
    F1rtt << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMicroSeconds() << std::endl;
}
void RTT2Change(Time oldRtt, Time newRtt) {
    F2rtt << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMicroSeconds() << std::endl;
}

void
connectF1 (Ptr <OnOffApplication > onoff)
{
    onoff->GetSocket()->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&Cwnd1Change));
    onoff->GetSocket()->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTT1Change));
}
void
connectF2 (Ptr <OnOffApplication > onoff)
{
    onoff->GetSocket()->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&Cwnd2Change));
    onoff->GetSocket()->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTT2Change));
}

int main (int argc, char *argv[])
{
    std::string outputFilePath = ".";
    std::string tcpTypeId = "DefTcpDccs";
    bool enableSwitchEcn = true;
    Time progressInterval = MilliSeconds (100);

    CommandLine cmd (__FILE__);
    cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
    cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
    cmd.Parse (argc, argv);

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));

    Time startTime = Seconds (0);
    Time stopTime = Seconds (2);
    Ptr<Node> S1 = CreateObject<Node> ();
    Ptr<Node> S2 = CreateObject<Node> ();
    Ptr<Node> S3 = CreateObject<Node> ();
    Ptr<Node> S4 = CreateObject<Node> ();
    Ptr<Node> T1 = CreateObject<Node> ();
    Ptr<Node> R1 = CreateObject<Node> ();

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1460));
    Config::SetDefault("ns3::TcpSocketBase::Timestamp", BooleanValue(false));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue(MilliSeconds(10)));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    
    Config::SetDefault("ns3::DefTcpDccs::NumFlows", UintegerValue(4));

    // Set default parameters for RED queue disc
    Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
    // ARED may be used but the queueing delays will increase; it is disabled
    // here because the SIGCOMM paper did not mention it
    Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("250p")));
    // DCTCP tracks instantaneous queue length only; so set QW = 1
    Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (50));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (50));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("10us"));
    pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("250p"));
    

    NetDeviceContainer T1S1 = pointToPoint.Install (T1, S1);
    NetDeviceContainer T1S2 = pointToPoint.Install (T1, S2);
    NetDeviceContainer T1S3 = pointToPoint.Install (T1, S3);
    NetDeviceContainer T1S4 = pointToPoint.Install (T1, S4);
    NetDeviceContainer R1T1 = pointToPoint.Install (R1, T1);


    InternetStackHelper stack;
    stack.InstallAll ();

    TrafficControlHelper tchRed;
    // MinTh = 50, MaxTh = 150 recommended in ACM SIGCOMM 2010 DCTCP Paper
    // This yields a target (MinTh) queue depth of 60us at 10 Gb/s
    tchRed.SetRootQueueDisc ("ns3::RedQueueDisc",
                             "LinkBandwidth", StringValue ("10Gbps"),
                             "LinkDelay", StringValue ("10us"),
                             "MinTh", DoubleValue (50),
                             "MaxTh", DoubleValue (50));
                             
    QueueDiscContainer queueDiscs1 = tchRed.Install (R1T1);
    queueDiscs1.Get(1)->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&QueueLengthTracet1));
    tchRed.Install (T1S1);
    tchRed.Install (T1S2);
    tchRed.Install (T1S3);
    tchRed.Install (T1S4);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ipR1T1 = address.Assign (R1T1);
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ipT1S1 = address.Assign (T1S1);
    address.NewNetwork ();
    //address.SetBase ("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipT1S2 = address.Assign (T1S2);
    address.NewNetwork ();
    //address.SetBase ("10.3.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipT1S3 = address.Assign (T1S3);
    address.NewNetwork ();
    //address.SetBase ("10.4.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipT1S4 = address.Assign (T1S4);
    address.NewNetwork ();
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    uint16_t port1 = 50000;
    uint16_t port2 = 50001;
    uint16_t port3 = 50002;
    uint16_t port4 = 50003;
    
    Address sinkLocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port1));
    Address sinkLocalAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
    Address sinkLocalAddress3 (InetSocketAddress (Ipv4Address::GetAny (), port3));
    Address sinkLocalAddress4 (InetSocketAddress (Ipv4Address::GetAny (), port4));
    
    PacketSinkHelper sinkHelper1 ("ns3::TcpSocketFactory", sinkLocalAddress1);
    PacketSinkHelper sinkHelper2 ("ns3::TcpSocketFactory", sinkLocalAddress2);
    PacketSinkHelper sinkHelper3 ("ns3::TcpSocketFactory", sinkLocalAddress3);
    PacketSinkHelper sinkHelper4 ("ns3::TcpSocketFactory", sinkLocalAddress4);
    
    ApplicationContainer sinkApp1 = sinkHelper1.Install (R1);
    ApplicationContainer sinkApp2 = sinkHelper2.Install (R1);
    ApplicationContainer sinkApp3 = sinkHelper3.Install (R1);
    ApplicationContainer sinkApp4 = sinkHelper4.Install (R1);
    
    Ptr<PacketSink> s1r1Sinks = sinkApp1.Get (0)->GetObject<PacketSink> ();
    Ptr<PacketSink> s2r1Sinks = sinkApp2.Get (0)->GetObject<PacketSink> ();
    Ptr<PacketSink> s3r1Sinks = sinkApp3.Get (0)->GetObject<PacketSink> ();
    Ptr<PacketSink> s4r1Sinks = sinkApp4.Get (0)->GetObject<PacketSink> ();
    
    sinkApp1.Start (startTime);
    sinkApp1.Stop (stopTime);
    sinkApp2.Start (startTime);
    sinkApp2.Stop (stopTime);
    sinkApp3.Start (startTime);
    sinkApp3.Stop (stopTime);
    sinkApp4.Start (startTime);
    sinkApp4.Stop (stopTime);

    OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
    clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
    clientHelper1.SetAttribute ("PacketSize", UintegerValue (1460));

    OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
    clientHelper2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
    clientHelper2.SetAttribute ("PacketSize", UintegerValue (1460));
    
    OnOffHelper clientHelper3 ("ns3::TcpSocketFactory", Address ());
    clientHelper3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper3.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
    clientHelper3.SetAttribute ("PacketSize", UintegerValue (1460));
    
    OnOffHelper clientHelper4 ("ns3::TcpSocketFactory", Address ());
    clientHelper4.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper4.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper4.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
    clientHelper4.SetAttribute ("PacketSize", UintegerValue (1460));

    ApplicationContainer clientApps1;
    AddressValue remoteAddress1 (InetSocketAddress (ipR1T1.GetAddress (0), port1));
    clientHelper1.SetAttribute ("Remote", remoteAddress1);
    clientApps1.Add (clientHelper1.Install (S1));
    clientApps1.Start (Seconds(0));
    clientApps1.Stop (stopTime);

    ApplicationContainer clientApps2;
    AddressValue remoteAddress2 (InetSocketAddress (ipR1T1.GetAddress (0), port2));
    clientHelper2.SetAttribute ("Remote", remoteAddress2);
    clientApps2.Add (clientHelper2.Install (S2));
    clientApps2.Start (Seconds(0));
    clientApps2.Stop (stopTime);

    ApplicationContainer clientApps3;
    AddressValue remoteAddress3 (InetSocketAddress (ipR1T1.GetAddress (0), port3));
    clientHelper3.SetAttribute ("Remote", remoteAddress3);
    clientApps3.Add (clientHelper3.Install (S3));
    clientApps3.Start (Seconds(0));
    clientApps3.Stop (stopTime);
    
    ApplicationContainer clientApps4;
    AddressValue remoteAddress4 (InetSocketAddress (ipR1T1.GetAddress (0), port4));
    clientHelper4.SetAttribute ("Remote", remoteAddress4);
    clientApps4.Add (clientHelper4.Install (S4));
    clientApps4.Start (Seconds(0.2));
    clientApps4.Stop (stopTime);
    
    F1cwnd.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F1cwnd", std::ios::out);
    F2cwnd.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F2cwnd", std::ios::out);
    F1Throughput.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F1Throughput", std::ios::out);
    F2Throughput.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F2Throughput", std::ios::out);
    F1rtt.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F1rtt", std::ios::out);
    F2rtt.open ("scratch/Convergence_Final/defective_dccs_conv/traces/F2rtt", std::ios::out);
    t1QueueLength.open ("dctcp-example-t1-length.dat", std::ios::out);
    t1QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;

    s2r1Sinks->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceF1Sink));
    Ptr <OnOffApplication > onoff = DynamicCast<OnOffApplication> (S3->GetApplication(0));
    Simulator::Schedule (Seconds(0.0000001), &connectF1, onoff);
    s4r1Sinks->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceF2Sink));
    onoff = DynamicCast<OnOffApplication> (S4->GetApplication(0));
    Simulator::Schedule (Seconds(0.2000001), &connectF2, onoff);

    
    Simulator::Schedule (progressInterval, &PrintProgress, progressInterval);
    Simulator::Stop (stopTime + TimeStep (1));

    Simulator::Run ();
    t1QueueLength.close ();
    Simulator::Destroy ();
    return 0;
}

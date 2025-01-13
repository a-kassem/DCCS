 #include <iostream>
 #include <iomanip>
 
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/traffic-control-module.h"
 #include "ns3/flow-monitor-module.h"
 #include "ns3/random-variable-stream.h"

 
uint64_t s1 = 50;
uint64_t s2 = 30;
uint64_t s3 = 100;

 using namespace ns3;
 
 std::ofstream rxS1R1Throughput;
 std::ofstream rxS2R2Throughput;
 std::ofstream rxS3R1Throughput;
 std::ofstream S1cwnd;
 std::ofstream S2cwnd;
 std::ofstream S3cwnd;
 std::ofstream S1rtt;
 std::ofstream S2rtt;
 std::ofstream S3rtt;
 std::ofstream fairnessIndex;
 std::ofstream t1QueueLength;
 std::ofstream t2QueueLength;
 std::ofstream r1QueueLength;
 std::ofstream convergenceTimeOutput;
 std::vector<uint64_t> rxS1R1Bytes;
 std::vector<uint64_t> rxS2R2Bytes;
 std::vector<uint64_t> rxS3R1Bytes;
 NodeContainer S1, S2, S3, R2;
 
 
 void
 PrintProgress (Time interval)
 {
   std::cout << "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << " seconds simulation time" << std::endl;
   Simulator::Schedule (interval, &PrintProgress, interval);
 }
 
 void
 TraceS1R1Sink (std::size_t index, Ptr<const Packet> p, const Address& a)
 {
   rxS1R1Bytes[index] += p->GetSize ();
 }
 
 void
 TraceS2R2Sink (std::size_t index, Ptr<const Packet> p, const Address& a)
 {
   rxS2R2Bytes[index] += p->GetSize ();
 }
 
 void
 TraceS3R1Sink (std::size_t index, Ptr<const Packet> p, const Address& a)
 {
   if(index == 0)
   {
   	S3cwnd << Simulator::Now().GetSeconds() << ":\t" << p->GetSize () << "Bytes Recieved" << std::endl;
   }
   rxS3R1Bytes[index] += p->GetSize ();
 }
 
 void
 InitializeCounters (void)
 {
   for (std::size_t i = 0; i < s1; i++)
     {
       rxS1R1Bytes[i] = 0;
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       rxS2R2Bytes[i] = 0;
     }
   for (std::size_t i = 0; i < s3; i++)
     {
       rxS3R1Bytes[i] = 0;
     }
 }
 
 void
PrintThroughput(Time measurementWindow)
{
    for (std::size_t i = 0; i < s1; i++)
    {
        rxS1R1Throughput << Simulator::Now().GetSeconds() << "s " << i << " "
                         << (rxS1R1Bytes[i] * 8) / (measurementWindow.GetSeconds()) / 1e6
                         << std::endl;
    }
    for (std::size_t i = 0; i < s2; i++)
    {
        rxS2R2Throughput << Simulator::Now().GetSeconds() << "s " << i << " "
                         << (rxS2R2Bytes[i] * 8) / (measurementWindow.GetSeconds()) / 1e6
                         << std::endl;
    }
    for (std::size_t i = 0; i < s3; i++)
    {
        rxS3R1Throughput << Simulator::Now().GetSeconds() << "s " << i << " "
                         << (rxS3R1Bytes[i] * 8) / (measurementWindow.GetSeconds()) / 1e6
                         << std::endl;
    }
}
 
 // Jain's fairness index:  https://en.wikipedia.org/wiki/Fairness_measure
 void
 PrintFairness (Time measurementWindow)
 {
   uint64_t sumSquares = 0;
   uint64_t sum = 0;
   double fairness = 0;
   for (std::size_t i = 0; i < s1; i++)
     {
       sum += rxS1R1Bytes[i];
       sumSquares += (rxS1R1Bytes[i] * rxS1R1Bytes[i]);
     }
   fairness = static_cast<double> (sum * sum) / (s1 * sumSquares);
   fairnessIndex << "Fairness for S1-R1 flows: "
                 << std::fixed << std::setprecision (3) << fairness << std::endl;
   sumSquares = 0;
   sum = 0;
   fairness = 0;
   for (std::size_t i = 0; i < s2; i++)
     {
       sum += rxS2R2Bytes[i];
       sumSquares += (rxS2R2Bytes[i] * rxS2R2Bytes[i]);
     }
   fairness = static_cast<double> (sum * sum) / (s2 * sumSquares);
   fairnessIndex << "Fairness for S2-R2 flows: "
                 << std::fixed << std::setprecision (3) << fairness << std::endl;
   sumSquares = 0;
   sum = 0;
   fairness = 0;
   for (std::size_t i = 0; i < s3; i++)
     {
       sum += rxS3R1Bytes[i];
       sumSquares += (rxS3R1Bytes[i] * rxS3R1Bytes[i]);
     }
   fairness = static_cast<double> (sum * sum) / (s3 * sumSquares);
   fairnessIndex << "Fairness for S3-R1 flows: "
                 << std::fixed << std::setprecision (3) << fairness << std::endl;
   sum = 0;
   for (std::size_t i = 0; i < s1; i++)
     {
       sum += rxS1R1Bytes[i];
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       sum += rxS2R2Bytes[i];
     }
   fairnessIndex << "Aggregate user-level throughput for flows through T1: " << static_cast<double> (sum * 8) / (measurementWindow.GetSeconds()) / 1e9 << " Gbps" << std::endl;
   sum = 0;
   for (std::size_t i = 0; i < s3; i++)
     {
       sum += rxS3R1Bytes[i];
     }
   for (std::size_t i = 0; i < s1; i++)
     {
       sum += rxS1R1Bytes[i];
     }
   fairnessIndex << "Aggregate user-level throughput for flows to R1: " << static_cast<double> (sum * 8) / (measurementWindow.GetSeconds()) / 1e9 << " Gbps" << std::endl;
 }
 
 void QueueLengthTracet1(uint32_t oldValue, uint32_t newValue)
{
   t1QueueLength << Simulator::Now().GetSeconds() << "\t" << newValue << std::endl;
}

 void QueueLengthTracet2(uint32_t oldValue, uint32_t newValue)
{
   t2QueueLength << Simulator::Now().GetSeconds() << "\t" << newValue << std::endl;
}

 void QueueLengthTracer1(uint32_t oldValue, uint32_t newValue)
{
   r1QueueLength << Simulator::Now().GetSeconds() << "\t" << newValue << std::endl;
}
 
 void Cwnd1Change(uint32_t oldCwnd, uint32_t newCwnd) {
    S1cwnd << Simulator::Now().GetSeconds() << "\t" << round(newCwnd/1448) << std::endl;
}
 void Cwnd2Change(uint32_t oldCwnd, uint32_t newCwnd) {
    S2cwnd << Simulator::Now().GetSeconds() << "\t" << round(newCwnd/1448) << std::endl;
}
 void Cwnd3Change(uint32_t oldCwnd, uint32_t newCwnd) {
    S3cwnd << Simulator::Now().GetSeconds() << "\t" << round(newCwnd/1448) << std::endl;
}

 void RTT1Change(Time oldRtt, Time newRtt) {
    S1rtt << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMicroSeconds() << std::endl;
}
 void RTT2Change(Time oldRtt, Time newRtt) {
    S2rtt << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMicroSeconds() << std::endl;
}
 void RTT3Change(Time oldRtt, Time newRtt) {
    S3rtt << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMicroSeconds() << std::endl;
}

  void
 connectS1 (Ptr <OnOffApplication > onoff)
 {
   onoff->GetSocket()->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&Cwnd1Change));
   onoff->GetSocket()->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTT1Change));
 }
 void
 connectS2 (Ptr <OnOffApplication > onoff)
 {
   onoff->GetSocket()->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&Cwnd2Change));
   onoff->GetSocket()->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTT2Change));
 }
 void
 connectS3 (Ptr <OnOffApplication > onoff)
 {
   onoff->GetSocket()->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&Cwnd3Change));
   onoff->GetSocket()->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTT3Change));
 }
 
 int main (int argc, char *argv[])
 {
   std::string outputFilePath = ".";
   std::string tcpTypeId = "TcpDctcp";
   Time flowStartupWindow = Seconds (0.5);
   Time convergenceTime = Seconds (1);
   Time measurementWindow = Seconds (0.5);
   bool enableSwitchEcn = true;
   Time progressInterval = MilliSeconds (100);
 
   CommandLine cmd (__FILE__);
   cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
   cmd.AddValue ("flowStartupWindow", "startup time window (TCP staggered starts)", flowStartupWindow);
   cmd.AddValue ("convergenceTime", "convergence time", convergenceTime);
   cmd.AddValue ("measurementWindow", "measurement window", measurementWindow);
   cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
   cmd.Parse (argc, argv);
 
   Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
 
   Time startTime = Seconds (0);
   Time stopTime = flowStartupWindow + convergenceTime + measurementWindow;
 
   Time clientStartTime = startTime;
 
   rxS1R1Bytes.reserve (s1);
   rxS2R2Bytes.reserve (s2);
   rxS3R1Bytes.reserve (s3);
 
   S1.Create (s1);
   S2.Create (s2);
   S3.Create (s3);
   R2.Create (s2);
   Ptr<Node> T1 = CreateObject<Node> ();
   Ptr<Node> T2 = CreateObject<Node> ();
   Ptr<Node> R1 = CreateObject<Node> ();
 
   Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1460));
   Config::SetDefault("ns3::TcpSocketBase::Timestamp", BooleanValue(false));
   Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue(MilliSeconds(5)));
   //Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
   GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
 
   // Set default parameters for RED queue disc
   Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
   // ARED may be used but the queueing delays will increase; it is disabled
   // here because the SIGCOMM paper did not mention it
   // Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
   // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
   Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
   Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
   Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("250p")));
   // DCTCP tracks instantaneous queue length only; so set QW = 1
   Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
   Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (50));
   Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (50));
 
   PointToPointHelper pointToPointSR;
   pointToPointSR.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
   pointToPointSR.SetChannelAttribute ("Delay", StringValue ("10us"));
   pointToPointSR.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("250p"));
 
   PointToPointHelper pointToPointT;
   pointToPointT.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
   pointToPointT.SetChannelAttribute ("Delay", StringValue ("10us"));
   pointToPointT.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("250p"));
 
 
   std::vector<NetDeviceContainer> S1T1;
   S1T1.reserve (s1);
   std::vector<NetDeviceContainer> S2T1;
   S2T1.reserve (s2);
   std::vector<NetDeviceContainer> S3T2;
   S3T2.reserve (s3);
   std::vector<NetDeviceContainer> R2T2;
   R2T2.reserve (s2);
   NetDeviceContainer T1T2 = pointToPointT.Install (T1, T2);
   NetDeviceContainer R1T2 = pointToPointSR.Install (R1, T2);
 
   for (std::size_t i = 0; i < s1; i++)
     {
       Ptr<Node> n = S1.Get (i);
       S1T1.push_back (pointToPointSR.Install (n, T1));
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       Ptr<Node> n = S2.Get (i);
       S2T1.push_back (pointToPointSR.Install (n, T1));
     }
   for (std::size_t i = 0; i < s3; i++)
     {
       Ptr<Node> n = S3.Get (i);
       S3T2.push_back (pointToPointSR.Install (n, T2));
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       Ptr<Node> n = R2.Get (i);
       R2T2.push_back (pointToPointSR.Install (n, T2));
     }
 
   InternetStackHelper stack;
   stack.InstallAll ();
 
   TrafficControlHelper tchRed10;
   tchRed10.SetRootQueueDisc ("ns3::RedQueueDisc",
                              "LinkBandwidth", StringValue ("10Gbps"),
                              "LinkDelay", StringValue ("10us"),
                              "MinTh", DoubleValue (50),
                              "MaxTh", DoubleValue (50));
   QueueDiscContainer queueDiscs1 = tchRed10.Install (T1T2);
   queueDiscs1.Get(0)->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&QueueLengthTracet1));
 
   TrafficControlHelper tchRed1;
   tchRed1.SetRootQueueDisc ("ns3::RedQueueDisc",
                             "LinkBandwidth", StringValue ("10Gbps"),
                             "LinkDelay", StringValue ("10us"),
                             "MinTh", DoubleValue (50),
                             "MaxTh", DoubleValue (50));
   QueueDiscContainer queueDiscs2 = tchRed1.Install (R1T2.Get (1));
   queueDiscs2.Get(0)->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&QueueLengthTracet2));
   QueueDiscContainer queueDiscs3 = tchRed1.Install (R1T2.Get (0));
   queueDiscs3.Get(0)->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&QueueLengthTracer1));
   for (std::size_t i = 0; i < s1; i++)
     {
       tchRed1.Install (S1T1[i].Get (1));
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       tchRed1.Install (S2T1[i].Get (1));
     }
   for (std::size_t i = 0; i < s3; i++)
     {
       tchRed1.Install (S3T2[i].Get (1));
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       tchRed1.Install (R2T2[i].Get (1));
     }
   Ipv4AddressHelper address;
   std::vector<Ipv4InterfaceContainer> ipS1T1;
   ipS1T1.reserve (s1);
   std::vector<Ipv4InterfaceContainer> ipS2T1;
   ipS2T1.reserve (s2);
   std::vector<Ipv4InterfaceContainer> ipS3T2;
   ipS3T2.reserve (s3);
   std::vector<Ipv4InterfaceContainer> ipR2T2;
   ipR2T2.reserve (s2);
   address.SetBase ("172.16.1.0", "255.255.255.0");
   Ipv4InterfaceContainer ipT1T2 = address.Assign (T1T2);
   address.SetBase ("192.168.0.0", "255.255.255.0");
   Ipv4InterfaceContainer ipR1T2 = address.Assign (R1T2);
   address.SetBase ("10.1.1.0", "255.255.255.0");
   for (std::size_t i = 0; i < s1; i++)
     {
       ipS1T1.push_back (address.Assign (S1T1[i]));
       address.NewNetwork ();
     }
   address.SetBase ("10.2.1.0", "255.255.255.0");
   for (std::size_t i = 0; i < s2; i++)
     {
       ipS2T1.push_back (address.Assign (S2T1[i]));
       address.NewNetwork ();
     }
   address.SetBase ("10.3.1.0", "255.255.255.0");
   for (std::size_t i = 0; i < s3; i++)
     {
       ipS3T2.push_back (address.Assign (S3T2[i]));
       address.NewNetwork ();
     }
   address.SetBase ("10.4.1.0", "255.255.255.0");
   for (std::size_t i = 0; i < s2; i++)
     {
       ipR2T2.push_back (address.Assign (R2T2[i]));
       address.NewNetwork ();
     }
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
   
   Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
   x->SetAttribute("Mean", DoubleValue(0.167));
   x->SetAttribute("Bound", DoubleValue(0.5));
 
   // Each sender in S2 sends to a receiver in R2
   
   std::vector<Ptr<PacketSink> > r2Sinks;
   r2Sinks.reserve (s2);
   for (std::size_t i = 0; i < s2; i++)
     {
       uint16_t port = 50000 + i;
       Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
       PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
       ApplicationContainer sinkApp = sinkHelper.Install (R2.Get (i));
       Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
       r2Sinks.push_back (packetSink);
       sinkApp.Start (startTime);
       sinkApp.Stop (stopTime);
 
       OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
       clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
       clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
       clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
       clientHelper1.SetAttribute ("PacketSize", UintegerValue (1460));
 
       ApplicationContainer clientApps1;
       AddressValue remoteAddress (InetSocketAddress (ipR2T2[i].GetAddress (0), port));
       clientHelper1.SetAttribute ("Remote", remoteAddress);
       double rand = x->GetValue();
       clientApps1.Add (clientHelper1.Install (S2.Get (i)));
       clientApps1.Start (Seconds(rand));
       clientApps1.Stop (stopTime);
     }
 

   // Each sender in S1 and S3 sends to R1
   std::vector<Ptr<PacketSink> > s1r1Sinks;
   std::vector<Ptr<PacketSink> > s3r1Sinks;
   s1r1Sinks.reserve (s1);
   s3r1Sinks.reserve (s3);
   for (std::size_t i = 0; i < s1 + s3; i++)
     {
       uint16_t port = 50000 + i;
       Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
       PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
       ApplicationContainer sinkApp = sinkHelper.Install (R1);
       Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
       if (i < s1)
         {
           s1r1Sinks.push_back (packetSink);
         }
       else 
         {
           s3r1Sinks.push_back (packetSink);
         }

       sinkApp.Start (startTime);
       sinkApp.Stop (stopTime);
 
       OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
       clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
       clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
       clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
       clientHelper1.SetAttribute ("PacketSize", UintegerValue (1460));
 
       ApplicationContainer clientApps1;
       AddressValue remoteAddress (InetSocketAddress (ipR1T2.GetAddress (0), port));
       clientHelper1.SetAttribute ("Remote", remoteAddress);
       if (i < s1)
         {
           clientApps1.Add (clientHelper1.Install (S1.Get (i)));
           clientApps1.Start (Seconds(x->GetValue()));
         }
       else 
         {
            clientApps1.Add (clientHelper1.Install (S3.Get (i - s1)));
            clientApps1.Start (Seconds(x->GetValue()));
         }
 
       clientApps1.Stop (stopTime);
     }
    
   S1cwnd.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S1cwnd", std::ios::out);
   S2cwnd.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S2cwnd", std::ios::out);
   S3cwnd.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S3cwnd", std::ios::out);
   S1rtt.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S1rtt", std::ios::out);
   S2rtt.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S2rtt", std::ios::out);
   S3rtt.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/S3rtt", std::ios::out);
   rxS1R1Throughput.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/s1-r1-throughput.dat", std::ios::out);
   rxS1R1Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
   rxS2R2Throughput.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/s2-r2-throughput.dat", std::ios::out);
   rxS2R2Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
   rxS3R1Throughput.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/s3-r1-throughput.dat", std::ios::out);
   rxS3R1Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
   fairnessIndex.open ("scratch/Fairness_Final/output/defective_dctcp_fairness.dat", std::ios::out);
   t1QueueLength.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/t1-length.dat", std::ios::out);
   t1QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;
   t2QueueLength.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/t2-length.dat", std::ios::out);
   t2QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;
   r1QueueLength.open ("scratch/Fairness_Final/defective_dctcp_fairness/traces/r1-length.dat", std::ios::out);
   r1QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;
   convergenceTimeOutput.open("convergenceTime.dat", std::ios::out);
   for (std::size_t i = 0; i < s1; i++)
     {
       s1r1Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS1R1Sink, i));
       Ptr <OnOffApplication > onoff = DynamicCast<OnOffApplication> (S1.Get(i)->GetApplication(0));
       Simulator::Schedule (Seconds(0.5), &connectS1, onoff);
     }
   for (std::size_t i = 0; i < s2; i++)
     {
       r2Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS2R2Sink, i));
       Ptr <OnOffApplication > onoff = DynamicCast<OnOffApplication> (S2.Get(i)->GetApplication(0));
       Simulator::Schedule (Seconds(0.5), &connectS2, onoff);
     }
   for (std::size_t i = 0; i < s3; i++)
     {
       s3r1Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS3R1Sink, i));
       Ptr <OnOffApplication > onoff = DynamicCast<OnOffApplication> (S3.Get(i)->GetApplication(0));
       if(i == 0)
       {
       	     Simulator::Schedule (Seconds(0.5), &connectS3, onoff);
       }
     }
    
   Simulator::Schedule (flowStartupWindow + convergenceTime, &InitializeCounters);
   Simulator::Schedule (flowStartupWindow + convergenceTime + measurementWindow, &PrintThroughput, measurementWindow);
   Simulator::Schedule (flowStartupWindow + convergenceTime + measurementWindow, &PrintFairness, measurementWindow);
   Simulator::Schedule (progressInterval, &PrintProgress, progressInterval);
   Simulator::Stop (stopTime + TimeStep (1));
 
   Simulator::Run ();
   rxS1R1Throughput.close ();
   rxS2R2Throughput.close ();
   rxS3R1Throughput.close ();
   fairnessIndex.close ();
   t1QueueLength.close ();
   t2QueueLength.close ();
   r1QueueLength.close ();
   Simulator::Destroy ();
   return 0;
 }

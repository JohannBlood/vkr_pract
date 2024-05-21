#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
uint32_t checkTimes;     //!< Number of times the queues have been checked.
double avgQueueDiscSize; //!< Average QueueDisc size.
std::string pathOut = ".";
std::stringstream filePlotQueueDisc;    //!< Output file name for queue disc size.
std::stringstream filePlotQueueDiscAvg; //!< Output file name for queue disc average.

Ipv4InterfaceContainer i0i0; //!< IPv4 interface container i0 + i2.
Ipv4InterfaceContainer i1i1; //!< IPv4 interface container i1 + i2.
Ipv4InterfaceContainer i2i2; //!< IPv4 interface container i2 + i3.
Ipv4InterfaceContainer i3i3; //!< IPv4 interface container i3 + i4.
Ipv4InterfaceContainer i0i1; //!< IPv4 interface container i3 + i5.
Ipv4InterfaceContainer i1i2; //!< IPv4 interface container i2 + i3.
Ipv4InterfaceContainer i2i3; //!< IPv4 interface container i3 + i4.
Ipv4InterfaceContainer i3i4; //!< IPv4 interface container i3 + i5.
uint64_t totalBytesSent = 0;
uint64_t totalPacketsSent = 0;

void
TxCallback (Ptr<const Packet> packet)
{
  totalBytesSent += packet->GetSize();
  totalPacketsSent++;
  NS_LOG_UNCOND ("Packet sent. Total bytes sent: " << totalBytesSent << ", total packets sent: " << totalPacketsSent);
}

/**
 * Check the queue disc size and write its stats to the output files.
 *
 * \param queue The queue to check.
 */
void
CheckQueueDiscSize(Ptr<QueueDisc> queue)
{
    uint32_t qSize = queue->GetCurrentSize().GetValue();

    avgQueueDiscSize += qSize;
    checkTimes++;

    // check queue disc size every 1/100 of a second
    Simulator::Schedule(Seconds(0.01), &CheckQueueDiscSize, queue);

    std::ofstream fPlotQueueDisc(filePlotQueueDisc.str(), std::ios::out | std::ios::app);
    fPlotQueueDisc << Simulator::Now().GetSeconds() << " " << qSize << std::endl;
    fPlotQueueDisc.close();

    std::ofstream fPlotQueueDiscAvg(filePlotQueueDiscAvg.str(), std::ios::out | std::ios::app);
    fPlotQueueDiscAvg << Simulator::Now().GetSeconds() << " " << avgQueueDiscSize / checkTimes
                      << std::endl;
    fPlotQueueDiscAvg.close();
}

void
PacketDropCallback (Ptr<const Packet> p)
{
  static uint32_t dropCount = 0;
  dropCount++;
  NS_LOG_UNCOND ("Packet dropped: " << dropCount);
}

int main (int argc, char *argv[])
{
  // Устанавливаем уровень логгирования
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  Config::SetDefault("ns3::RedQueueDisc::MaxSize", StringValue("25p"));

  // Создаем три узла
  NodeContainer nodes;
  nodes.Create (5);

  // Создаем три узла
  NodeContainer sources;
  sources.Create (4);

  // Устанавливаем стек интернет-протоколов
  InternetStackHelper stack;
  stack.Install (nodes);
  stack.Install (sources);

  std::string aredLinkDataRate = "5Mbps";
  std::string aredLinkDelay = "2ms";

  // TrafficControlHelper tchRed;
  // uint16_t handle = tchRed.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue("100p"));
  // tchRed.AddInternalQueues(handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue("800p"));
  TrafficControlHelper tchRed;
  tchRed.SetRootQueueDisc("ns3::RedQueueDisc",
                            "LinkBandwidth",
                            StringValue(aredLinkDataRate),
                            "LinkDelay",
                            StringValue(aredLinkDelay));

  // Настраиваем первый канал "точка-точка"
  PointToPointHelper p2p;

  QueueDiscContainer queueDiscs;

  // Sources
  NetDeviceContainer s1r1;
  NetDeviceContainer s2r2;
  NetDeviceContainer s3r3;
  NetDeviceContainer s4r4;

  p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("25p"));
  p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  s1r1 = p2p.Install(sources.Get(0), nodes.Get(0));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  s2r2 = p2p.Install(sources.Get(1), nodes.Get(1));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  s3r3 = p2p.Install(sources.Get(2), nodes.Get(2));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  s4r4 = p2p.Install(sources.Get(3), nodes.Get(3));

  // Nodes
  NetDeviceContainer r1r2;
  NetDeviceContainer r2r3;
  NetDeviceContainer r3r4;
  NetDeviceContainer r4r5;
  p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("25p"));
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  r1r2 = p2p.Install (nodes.Get (0), nodes.Get (1));
  queueDiscs.Add(tchRed.Install(r1r2));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  r2r3 = p2p.Install (nodes.Get (1), nodes.Get (2));
  queueDiscs.Add(tchRed.Install(r2r3));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  r3r4 = p2p.Install (nodes.Get (2), nodes.Get (3));
  queueDiscs.Add(tchRed.Install(r3r4));

  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  r4r5 = p2p.Install (nodes.Get (3), nodes.Get (4));
  queueDiscs.Add(tchRed.Install(r4r5));

  // NS_LOG_INFO("Assign IP Addresses");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  i0i0 = ipv4.Assign(s1r1);

  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  i1i1 = ipv4.Assign(s2r2);

  ipv4.SetBase("10.1.3.0", "255.255.255.0");
  i2i2 = ipv4.Assign(s3r3);

  ipv4.SetBase("10.1.4.0", "255.255.255.0");
  i3i3 = ipv4.Assign(s4r4);

  ipv4.SetBase("10.1.5.0", "255.255.255.0");
  i0i1 = ipv4.Assign(r1r2);

  ipv4.SetBase("10.1.6.0", "255.255.255.0");
  i1i2 = ipv4.Assign(r2r3);

  ipv4.SetBase("10.1.7.0", "255.255.255.0");
  i2i3 = ipv4.Assign(r3r4);

  ipv4.SetBase("10.1.8.0", "255.255.255.0");
  i3i4 = ipv4.Assign(r4r5);

  // Устанавливаем маршрутизацию
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Создаем сервер-приложение на узле 2
  uint16_t port = 9;  // Порт для приложения
  Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (4));
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (30.0));

  // Создаем клиент-приложение OnOff на узле 0
  OnOffHelper source1 ("ns3::TcpSocketFactory", InetSocketAddress (i3i4.GetAddress (1), port));
  source1.SetAttribute("DataRate", DataRateValue(DataRate("30Mb/s")));
  source1.SetAttribute("MaxBytes", UintegerValue(1024 * 1024 * 1024));
  ApplicationContainer clientApps1;
  clientApps1.Add(source1.Install (sources.Get (0)));
  clientApps1.Start (Seconds (2.0));
  clientApps1.Stop (Seconds (29.0));

  // Создаем клиент-приложение OnOff на узле 0
  OnOffHelper source5 ("ns3::TcpSocketFactory", InetSocketAddress (i3i4.GetAddress (1), port));
  source5.SetAttribute("DataRate", DataRateValue(DataRate("30Mb/s")));
  source5.SetAttribute("MaxBytes", UintegerValue(1024 * 1024 * 1024));
  ApplicationContainer clientApps5;
  clientApps5.Add(source1.Install (sources.Get (0)));
  clientApps5.Start (Seconds (2.0));
  clientApps5.Stop (Seconds (29.0));

  // // Создаем клиент-приложение OnOff на узле 0
  // OnOffHelper source2 ("ns3::UdpSocketFactory", InetSocketAddress (i3i4.GetAddress (1), port));
  // source2.SetAttribute("DataRate", DataRateValue(DataRate("30Mb/s")));
  // source1.SetAttribute("MaxBytes", UintegerValue(1024 * 1024 * 1024));
  // ApplicationContainer clientApps2;
  // clientApps2.Add(source2.Install (sources.Get (1)));
  // clientApps2.Start (Seconds (2.0));
  // clientApps2.Stop (Seconds (9.0));

  // // Создаем клиент-приложение OnOff на узле 0
  // OnOffHelper source3 ("ns3::UdpSocketFactory", InetSocketAddress (i3i4.GetAddress (1), port));
  // source3.SetAttribute("DataRate", DataRateValue(DataRate("30Mb/s")));
  // source1.SetAttribute("MaxBytes", UintegerValue(1024 * 1024 * 1024));
  // ApplicationContainer clientApps3;
  // clientApps3.Add(source3.Install (sources.Get (2)));
  // clientApps3.Start (Seconds (2.0));
  // clientApps3.Stop (Seconds (9.0));

  // // Создаем клиент-приложение OnOff на узле 0
  // OnOffHelper source4 ("ns3::UdpSocketFactory", InetSocketAddress (i3i4.GetAddress (1), port));
  // source4.SetAttribute("DataRate", DataRateValue(DataRate("30Mb/s")));
  // source1.SetAttribute("MaxBytes", UintegerValue(1024 * 1024 * 1024));
  // ApplicationContainer clientApps4;
  // clientApps4.Add(source4.Install (sources.Get (3)));
  // clientApps4.Start (Seconds (2.0));
  // clientApps4.Stop (Seconds (9.0));

  // Настраиваем колбэк для отслеживания сброса пакетов
  // Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback (&PacketDropCallback));

  if (true)
  {
    filePlotQueueDisc << pathOut << "/"
                      << "stat_last.plotme";
    filePlotQueueDiscAvg << pathOut << "/"
                          << "stat_avg_last.plotme";

    remove(filePlotQueueDisc.str().c_str());
    remove(filePlotQueueDiscAvg.str().c_str());
    Ptr<QueueDisc> queue = queueDiscs.Get(6);
    Simulator::ScheduleNow(&CheckQueueDiscSize, queue);
  }

  Ptr<OnOffApplication> app = DynamicCast<OnOffApplication> (clientApps1.Get (0));
  app->TraceConnectWithoutContext ("Tx", MakeCallback (&TxCallback));

  Simulator::Stop(Seconds(30.0));
  // Запускаем симуляцию
  Simulator::Run ();
  for (int i = 0; i < queueDiscs.GetN(); i+=2) {
    QueueDisc::Stats st = queueDiscs.Get(i)->GetStats();
    std::cout << st << std::endl;
  }
  // Получаем количество полученных пакетов
  Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Packets Received: " << sink->GetTotalRx () << std::endl;
  std::cout << "Total Bytes Sent: " << totalBytesSent << std::endl;

  Simulator::Destroy ();

  return 0;
}



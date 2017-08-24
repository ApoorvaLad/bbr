/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
//
//       n0 ------n1(router)----- n2
//           
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.


// TODO
// addlogging to each node
// add queue to n1 and monitor the node
//maintain tests



#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");
int
main (int argc, char *argv[])
{


 

    LogComponentEnable("BulkSendApplication",LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink",LOG_LEVEL_INFO);


      Config::SetDefault("ns3::TcpL4Protocol::SocketType",StringValue("ns3::TcpBbr"));
       //  Config::SetDefault("ns3::TcpL4Protocol::SocketType",StringValue("ns3::TcpWestwood"));
       //Config::SetDefault("ns3::TcpL4Protocol::TcpTxBuffer", UintegerValue(50*1024));
       // Config::Set("ns3::TcpBbr::SetSndBufSize", UintegerValue(50*1024));
 // bool tracing = false;
       //uint32_t maxBytes =50000*1024; //*1024;
      uint32_t maxBytes =40000*1024;
     //uint32_t maxBytes = 1024;
 // double errRate = 0.00002;
 // double errRate = 0;
//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes, router;
  nodes.Create (2);
  router.Create(1);
  NS_LOG_INFO ("Create channels.");

  NodeContainer n0n1 = NodeContainer(nodes.Get(0),router.Get(0));
  NodeContainer n1n2 = NodeContainer(router.Get(0),nodes.Get(1));
  //
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("150Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  //pointToPoint.SetQueue("ns3::DropTailQueue");
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(2000));


  NetDeviceContainer devices,devices2;
  devices = pointToPoint.Install (n0n1);


  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(2000));

 // pointToPoint.SetQueue("ns3::DropTailQueue");

  devices2 = pointToPoint.Install (n1n2);
//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);
  internet.Install (router);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (devices);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign (devices2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Create Applications.");
/*  DoubleValue rate (errRate);
  Ptr<RateErrorModel> em1 = 
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"), "ErrorRate", rate);
  Ptr<RateErrorModel> em2 = 
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"), "ErrorRate", rate);

  // This enables the specified errRate on both link endpoints.
  devices.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em2));
  devices2.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
  devices2.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em2));*/
//
// Create a BulkSendApplication and install it on node 0
//
  uint16_t port = 9;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i1i2.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (150.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (150.0));

//
// Set up tracing if enabled
//
  
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      pointToPoint.EnablePcapAll ("tcp-bulk-send", true);
   

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (150.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}

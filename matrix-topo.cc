/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Egemen K. Cetinkaya, Justin P. Rohrer, and Amit Dandekar
 *
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
 *
 * Author: Egemen K. Cetinkaya <ekc@ittc.ku.edu>
 * Author: Justin P. Rohrer    <rohrej@ittc.ku.edu>
 * Author: Amit Dandekar       <dandekar@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center 
 * and
 * Department of Electrical Engineering and Computer Science
 * The University of Kansas
 * Lawrence, KS  USA
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture) and 
 * by NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI)
 *
 * This program reads an upper triangular adjacency matrix (e.g. adjacency_matrix.txt) and
 * node coordinates file (e.g. node_coordinates.txt). The program also set-ups a
 * wired network topology with P2P links according to the adjacency matrix with
 * nx(n-1) CBR traffic flows, in which n is the number of nodes in the adjacency matrix.
 */

// ---------- Header Includes -------------------------------------------------
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <map>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/assert.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/queue.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"


using namespace std;
using namespace ns3;

// ---------- Prototypes ------------------------------------------------------

vector<vector<bool> > readNxNMatrix (std::string adj_mat_file_name);
vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name);
void printCoordinateArray (const char* description, vector<vector<double> > coord_array);
void printMatrix (const char* description, vector<vector<bool> > array);
static void
RxDrop (Ptr<const Packet> p);
//static void
//CalculateDelay (Ptr<const Packet>p,const Address &address);
void print_packet_n();
void print_queue_length();
void output_element(int i);
NS_LOG_COMPONENT_DEFINE ("GenericTopologyCreation");
NodeContainer nodes;
int main (int argc, char *argv[])
{

  // ---------- Simulation Variables ------------------------------------------

  // Change the variables and file names only in this block!
  uint32_t computer_n  = 50;
  uint32_t router_n    = 30;

  double SimTime       = 120.00;
  double AppStartTime  = 0;
  double AppStopTime   = SimTime;
  uint32_t max_queue_length = 100000;

  //sending packets settings
  uint32_t MaxPacketSize = 210;
  Time interPacketInterval = Seconds (0.00375);
  uint32_t maxPacketCount = 1000;
  
  //link settings
  std::string Link_computer_router_Rate ("10Mbps");
  std::string Link_router_router_Rate ("0.5Mbps");
  std::string LinkDelay ("2ms");
  
  //error rate settings
  double error_rate=0; 


  srand ( (unsigned)time ( NULL ) );   // generate different seed each time

  std::string tr_name ("n-node-ppp.tr");
  std::string pcap_name ("n-node-ppp");
  std::string flow_name ("n-node-ppp.xml");
  std::string anim_name ("n-node-ppp.anim.xml");


  // the file address of input_adjacency matrix and node coordinate settings
  std::string adj_mat_file_name ("scratch/adjacency_matrix.txt");
  std::string node_coordinates_file_name ("scratch/node_coordinates.txt");

  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  // ---------- End of Simulation Variables ----------------------------------



  // ---------- Read Adjacency Matrix ----------------------------------------

  vector<vector<bool> > Adj_Matrix;
  Adj_Matrix = readNxNMatrix (adj_mat_file_name);

  // Optionally display 2-dimensional adjacency matrix (Adj_Matrix) array
  // printMatrix (adj_mat_file_name.c_str (),Adj_Matrix);

  // ---------- End of Read Adjacency Matrix ---------------------------------

  // ---------- Read Node Coordinates File -----------------------------------

  vector<vector<double> > coord_array;
  coord_array = readCordinatesFile (node_coordinates_file_name);

  // Optionally display node co-ordinates file
  // printCoordinateArray (node_coordinates_file_name.c_str (),coord_array);

  int n_nodes = coord_array.size ();
  int matrixDimension = Adj_Matrix.size ();

  if (matrixDimension != n_nodes)
    {
      NS_FATAL_ERROR ("The number of lines in coordinate file is: " << n_nodes << " not equal to the number of nodes in adjacency matrix size " << matrixDimension);
    }

  // ---------- End of Read Node Coordinates File ----------------------------

  // ---------- Network Setup ------------------------------------------------

  NS_LOG_INFO ("Create Nodes.");

     // Declare nodes objects
  nodes.Create (n_nodes);
  
  for(uint32_t i=0;i<computer_n;i++){
        stringstream ss;
        ss<<"computer"<<i+1;
        Names::Add (ss.str(), nodes.Get (i));
        }
  for(uint32_t i=0;i<router_n;i++){
        stringstream ss;
        ss<<"router"<<i+1;
        Names::Add (ss.str(), nodes.Get (i+computer_n));
        }

  NS_LOG_INFO ("Create P2P Link Attributes.");

  PointToPointHelper p2p_computer_router;
  PointToPointHelper p2p_router_router;
  p2p_computer_router.SetDeviceAttribute ("DataRate", StringValue (Link_computer_router_Rate));
  p2p_computer_router.SetChannelAttribute ("Delay", StringValue (LinkDelay));
  
  p2p_router_router.SetDeviceAttribute ("DataRate", StringValue (Link_router_router_Rate));
  p2p_router_router.SetChannelAttribute ("Delay", StringValue (LinkDelay));

  NS_LOG_INFO ("Install Internet Stack to Nodes.");

  InternetStackHelper internet;
  internet.Install (NodeContainer::GetGlobal ());

  NS_LOG_INFO ("Assign Addresses to Nodes.");

  Ipv4AddressHelper ipv4_n;
  ipv4_n.SetBase ("10.0.0.0", "255.255.255.0");

  NS_LOG_INFO ("Create Links Between Nodes.");

  uint32_t linkCount = 0;
  map<Ptr<NetDevice>,Ipv4Interface> map_dev_if;
  NetDeviceContainer n_devs;
  for (size_t i = 0; i < Adj_Matrix.size (); i++)
    {
      for (size_t j = 0; j < Adj_Matrix[i].size (); j++)
        {

          if (Adj_Matrix[i][j] == 1)
            {
              NodeContainer n_links = NodeContainer (nodes.Get (i), nodes.Get (j));
              if(i<computer_n) n_devs=p2p_computer_router.Install (n_links);
              else n_devs=p2p_router_router.Install(n_links);
              stringstream ss1,ss2,ss3,ss4;
              Ipv4InterfaceContainer ipv4_intctr;
              if(i<computer_n) ss1<<"computer"<<i+1;
              else ss1<<"router"<<i-computer_n+1;
              if(j<computer_n) ss2<<"computer"<<j+1;
              else ss2<<"router"<<j-computer_n+1;
              ss3<<ss1.str()<<"--"<<ss2.str();
              ss4<<ss2.str()<<"--"<<ss1.str();
              Names::Add (ss3.str(), n_devs.Get (0));
              Names::Add (ss4.str(), n_devs.Get (1));
              

          
              ipv4_intctr=ipv4_n.Assign (n_devs);
              ipv4_intctr.SetMetric(0,1);
              ipv4_intctr.SetMetric(1,1);
                          
              Ptr<RateErrorModel>em=CreateObject<RateErrorModel> ();
              em->SetAttribute("ErrorRate",DoubleValue(error_rate));
              n_devs.Get(1)->SetAttribute("ReceiveErrorModel",PointerValue (em));
              n_devs.Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

              ipv4_n.NewNetwork ();
              linkCount++;
              NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 1");
            }
          else
            {
              NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 0");
            }
        }
    }
  NS_LOG_INFO ("Number of links in the adjacency matrix is: " << linkCount);
  NS_LOG_INFO ("Number of all nodes is: " << nodes.GetN ());

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // ---------- End of Network Set-up ----------------------------------------

  // ---------- Allocate Node Positions --------------------------------------

  NS_LOG_INFO ("Allocate Positions to Nodes.");

  MobilityHelper mobility_n;
  Ptr<ListPositionAllocator> positionAlloc_n = CreateObject<ListPositionAllocator> ();

  for (size_t m = 0; m < coord_array.size (); m++)
    {
      positionAlloc_n->Add (Vector (coord_array[m][0], coord_array[m][1], 0));
      Ptr<Node> n0 = nodes.Get (m);
      Ptr<ConstantPositionMobilityModel> nLoc =  n0->GetObject<ConstantPositionMobilityModel> ();
      if (nLoc == 0)
        {
          nLoc = CreateObject<ConstantPositionMobilityModel> ();
          n0->AggregateObject (nLoc);
        }
      // y-coordinates are negated for correct display in NetAnim
      // NetAnim's (0,0) reference coordinates are located on upper left corner
      // by negating the y coordinates, we declare the reference (0,0) coordinate
      // to the bottom left corner
      Vector nVec (coord_array[m][0], -coord_array[m][1], 0);
      nLoc->SetPosition (nVec);

    }
  mobility_n.SetPositionAllocator (positionAlloc_n);
  mobility_n.Install (nodes);

  // ---------- End of Allocate Node Positions -------------------------------

  // ---------- Create n*(n-1) CBR Flows -------------------------------------

  NS_LOG_INFO ("Setup Packet Sinks with application Udpserver");

  uint16_t port = 9;
  ApplicationContainer apps_server;
  for (uint32_t i = 0; i < computer_n; i++)
    {
     
      UdpServerHelper server (port);
       ApplicationContainer apps = server.Install (nodes.Get (i));
       apps_server.Start (Seconds (AppStartTime));
       apps_server.Stop (Seconds (AppStopTime));
    }
  

  NS_LOG_INFO ("Setup Packet Sinks with application Udpclient.");


  // i_j_generator can generate random number of pairs with computer and router.        
  vector<int> i_j_generator;            
  for(uint32_t k=0;k<computer_n;k++)
        i_j_generator.push_back(k);
             
  for(uint32_t time=0;time<1200;time++){   //1200=120/0.1
        random_shuffle(i_j_generator.begin(), i_j_generator.end());
	int pair_number=rand()%(computer_n/2)+1;
        //pair_number varies :[1,25]
        for(int k=0;k<pair_number;k++){           
                int i=i_j_generator[2*k];
                int j=i_j_generator[2*k+1];
                    
                //get the node-i address            
                Ptr<Node> n = nodes.Get (i);
                Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
                Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
                Ipv4Address ip_addr = ipv4_int_addr.GetLocal ();
                
                
                UdpClientHelper client (ip_addr, port);
                client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
                client.SetAttribute ("Interval", TimeValue (interPacketInterval));
                client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));

                ApplicationContainer apps_client = client.Install (nodes.Get (j));

                apps_client.Start (Seconds (0.1*time));
                apps_client.Stop (Seconds (0.1*time+0.1));
                //the app above will send packets from node-j to node-i
        }
  }

  //Set the max queue length of each device in each node
  for(int i=0;i<n_nodes;i++){
        uint32_t n_devices=nodes.Get(i)->GetNDevices();
        for(uint32_t j=1;j<n_devices;j++){           
          nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->SetMaxPackets(max_queue_length);
       
                }
        }  
               
            
              

  
  

  // ---------- End of Create n*(n-1) CBR Flows ------------------------------

  // ---------- Simulation Monitoring ----------------------------------------

 // NS_LOG_INFO ("Configure Tracing.");

 // AsciiTraceHelper ascii;
  //p2p.EnableAsciiAll (ascii.CreateFileStream (tr_name.c_str ()));
   //p2p.EnablePcapAll (pcap_name.c_str());

 //  Ptr<FlowMonitor> flowmon;
 //  FlowMonitorHelper flowmonHelper;
 //  flowmon = flowmonHelper.InstallAll ();

  // Configure animator with default settings

 // AnimationInterface anim (anim_name.c_str ());
 NS_LOG_INFO ("Run Simulation.");
  for(float i=0;i<SimTime;i+=0.2)
        Simulator::Schedule (Seconds (i),&print_queue_length);
  //for(float i=0;i<SimTime;i+=0.2)
  //      Simulator::Schedule (Seconds (i),&print_packet_n);
  //Simulator::Schedule (Seconds(SimTime),&print_packet_n);
  Simulator::Stop (Seconds (SimTime));
  Simulator::Run ();
  // flowmon->SerializeToXmlFile (flow_name.c_str(), true, true);
  Simulator::Destroy ();

  // ---------- End of Simulation Monitoring ---------------------------------

  return 0;

}









// ---------- Function Definitions -------------------------------------------
void print_packet_n()
{       
        int all_packets=0;
        int n_devices=0;
        int max=0;
        for(int i=50;i<80;i++){
                all_packets=0;
                n_devices=nodes.Get(i)->GetNDevices();
                
                for(int j=1;j<n_devices;j++){
                        all_packets+=nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->GetTotalReceivedPackets();
                if(max<all_packets) max=all_packets;
                
                }
        
        }

        NS_LOG_UNCOND(Simulator::Now ().GetSeconds ()<<"\t "<<double(max)/Simulator::Now ().GetSeconds ());
}
void print_queue_length()
{
 // int n_packets=0;
  int all_packets=0;
  int max=0;
 // int all_packets=0; 
  int n_devices=0;
  for(int i=0;i<80;i++){
        n_devices=nodes.Get(i)->GetNDevices();
        all_packets=0;
        for(int j=1;j<n_devices;j++){
          all_packets+=nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->GetNPackets();
        }
        if(max<all_packets) max=all_packets;
   
        }
  NS_LOG_UNCOND(Simulator::Now ().GetSeconds ()<<"\t "<<max);
}
static void
RxDrop (Ptr<const Packet> p)
{
  static int show_count=0;
  static int n_drop_packets=0;
  n_drop_packets++;
  show_count++;
  if(show_count==100)
        {NS_LOG_UNCOND ( Simulator::Now ().GetSeconds ()<<"\t " <<double(n_drop_packets)/p->GetUid());
        show_count=0;}
  //
}



vector<vector<bool> > readNxNMatrix (std::string adj_mat_file_name)
{
  ifstream adj_mat_file;
  adj_mat_file.open (adj_mat_file_name.c_str (), ios::in);
  if (adj_mat_file.fail ())
    {
      NS_FATAL_ERROR ("File " << adj_mat_file_name.c_str () << " not found");
    }
  vector<vector<bool> > array;
  int i = 0;
  int n_nodes = 0;

  while (!adj_mat_file.eof ())
    {
      string line;
      getline (adj_mat_file, line);
      if (line == "")
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row in the array: " << i);
          break;
        }

      istringstream iss (line);
      bool element;
      vector<bool> row;
      int j = 0;

      while (iss >> element)
        {
          row.push_back (element);
          j++;
        }

      if (i == 0)
        {
          n_nodes = j;
        }

      if (j != n_nodes )
        {
          NS_LOG_ERROR ("ERROR: Number of elements in line " << i << ": " << j << " not equal to number of elements in line 0: " << n_nodes);
          NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
        }
      else
        {
          array.push_back (row);
        }
      i++;
    }

  if (i != n_nodes)
    {
      NS_LOG_ERROR ("There are " << i << " rows and " << n_nodes << " columns.");
      NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
    }

  adj_mat_file.close ();
  return array;

}

vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name)
{
  ifstream node_coordinates_file;
  node_coordinates_file.open (node_coordinates_file_name.c_str (), ios::in);
  if (node_coordinates_file.fail ())
    {
      NS_FATAL_ERROR ("File " << node_coordinates_file_name.c_str () << " not found");
    }
  vector<vector<double> > coord_array;
  int m = 0;

  while (!node_coordinates_file.eof ())
    {
      string line;
      getline (node_coordinates_file, line);

      if (line == "")
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row: " << m);
          break;
        }

      istringstream iss (line);
      double coordinate;
      vector<double> row;
      int n = 0;
      while (iss >> coordinate)
        {
          row.push_back (coordinate);
          n++;
        }

      if (n != 2)
        {
          NS_LOG_ERROR ("ERROR: Number of elements at line#" << m << " is "  << n << " which is not equal to 2 for node coordinates file");
          exit (1);
        }

      else
        {
          coord_array.push_back (row);
        }
      m++;
    }
  node_coordinates_file.close ();
  return coord_array;

}

void printMatrix (const char* description, vector<vector<bool> > array)
{
  cout << "**** Start " << description << "********" << endl;
  for (size_t m = 0; m < array.size (); m++)
    {
      for (size_t n = 0; n < array[m].size (); n++)
        {
          cout << array[m][n] << ' ';
        }
      cout << endl;
    }
  cout << "**** End " << description << "********" << endl;

}

void printCoordinateArray (const char* description, vector<vector<double> > coord_array)
{
  cout << "**** Start " << description << "********" << endl;
  for (size_t m = 0; m < coord_array.size (); m++)
    {
      for (size_t n = 0; n < coord_array[m].size (); n++)
        {
          cout << coord_array[m][n] << ' ';
        }
      cout << endl;
    }
  cout << "**** End " << description << "********" << endl;

}

// ---------- End of Function Definitions ------------------------------------

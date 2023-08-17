#include<bits/stdc++.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<thread>
#include <fcntl.h>
#include <chrono>
#include<dirent.h>
#include <openssl/md5.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>


using namespace std;






class Packet
{
    public:
    string filename;
    int uid;
    int depth; // depth >=2 not to be forwarded
    int from_port;

    Packet(string filename,int uid,int depth,int from_port)
    {
        this->filename=filename;
        this->uid=uid;
        this->depth=depth;
        this->from_port=from_port;
    }


};


//now i don't care who sends the packet i know that this packet originated from packet.uid
//member variables
int number_of_neighbours=0; //because something wrong with the param in thread
int listen_fd;
queue<Packet> Packet_to_forward; //add a packet to this queue only if depth is less than 2
vector< pair<int,int>> IMMEDIATE_NEIGHBOUR; //my immediate neighbours
vector<Packet> FILES_TO_SEARCH; //i need an int because i have to resolve tie and the depth is also stored here
//member methods                                   // first is the owner id second is the depth
vector<string> my_files;
//vector<Packet> FILES_TO_SEARCH;
map<int,int> neigh_uid_info; //port ---> uid
void forward_to_neighbour();
void receive_from_everyone();

bool compareFunction (string a, string b) {return a<b;} 
bool comparePacketFunction(Packet a,Packet b){return a.filename<b.filename;}


bool portsort (pair<int,int> a, pair<int,int> b) {return a.second<b.second;} 
void parse_directory(string directory_path,int &CLIENT_PORT,int &CLIENT_UID,
                queue<Packet> &Packet_to_forward,vector<string> &my_files);
//vector<string> split_string(string s);
void print_needed_file_info(vector<Packet> FILES_TO_SEARCH,string DIRECTORY_PATH);
int find_packet(vector<Packet> FILES_TO_SEARCH,string filename,int size);
string subsequence(char* msg,int start,int end);
void sending_files(vector<Packet> FILES_TO_SEARCH);


vector<string> split_string(string s)
{
    
    vector<string> data;
    string value="";
    for(int i=0;i<s.length();i++)
    {
        if(s[i] == ' '  )
        {
              data.push_back(value);
              value="";
        }
        else
           value += s[i];
    }
    if (!value.empty() && value[value.length()-1] == '\n') {
    value.erase(value.length()-1);
}
    data.push_back(value);

    return data;

}


vector<int> space_seperate_strings(string s)
{
     
    vector<int> data;
    string value="";
    for(int i=0;i<s.length();i++)
    {
        if(s[i] == ' '  )
        {

              data.push_back(stoi(value));
              value="";
        }
        else
           value += s[i];
    }
    
    if(!value.empty())
      data.push_back(stoi(value));
    return data;

}

void getdetails(string CONFIGURATION_FILE,string DIRECTORY_PATH,int &CLIENT_ID,int &CLIENT_UID,
                int & CLIENT_PORT,int &number_of_neighbours,vector< pair<int,int>> &IMMEDIATE_NEIGHBOUR
                ,queue<Packet> &Packet_to_forward,vector<Packet> &FILES_TO_SEARCH,vector<string> &my_files,map<int,int> &neigh_uid_info)
{
try
    {

      

        fstream my_file;
        my_file.open(CONFIGURATION_FILE, ios::in);

        if (!my_file)
        {
            std::cout << "Error in Configuration File ";
        }
        else
        {
            string line;
            
            getline(my_file, line);
            
            vector<int> client_data=space_seperate_strings(line);
           
            //collecting my data
            CLIENT_ID=client_data[0];
            
            CLIENT_PORT=client_data[1];
            
            CLIENT_UID=client_data[2];
            
            


            getline(my_file,line);
            number_of_neighbours=stoi(line);
           

            //get the neighbour info and store in the vector
            //get all the info
            getline(my_file,line);
            vector<int> neigh_data=space_seperate_strings(line);
            
            for(int i=0;i<neigh_data.size();i=i+2)
            {
                //cout<<"Neighbor of "<<CLIENT_PORT<<" is: "<<neigh_data[i+1]<<"\n";
                IMMEDIATE_NEIGHBOUR.push_back(make_pair(neigh_data[i],neigh_data[i+1]));
                neigh_uid_info.insert(make_pair(neigh_data[i+1],0));
                
            }
            sort(IMMEDIATE_NEIGHBOUR.begin(),IMMEDIATE_NEIGHBOUR.end(),portsort);
            getline(my_file,line);
            int NUM_FILES=stoi(line);
            
            for(int i=0;i<NUM_FILES;i++)
            {
                getline(my_file,line);
                // if(i!=NUM_FILES-1)
                //   line.erase(line.length()-1);
                Packet packet_d0=Packet(line,0,0,0);
                FILES_TO_SEARCH.push_back(packet_d0);
            }
            //cout<<FILES_TO_SEARCH.size()<<endl;
           
            my_file.close();
        }

        parse_directory(DIRECTORY_PATH,CLIENT_PORT,CLIENT_UID,Packet_to_forward,my_files);
    }  catch (exception e)
    {
        std::cout << "INPUT ERROR" << endl;
    }
}





//pass by value so that it gets reflected in the original cpp file
void parse_directory(string directory_path,int &CLIENT_PORT,int &CLIENT_UID,
                queue<Packet> &Packet_to_forward,vector<string> &my_files)
{
 struct dirent *d;
 DIR *dr;
 dr =opendir(directory_path.c_str());
 if(dr != NULL)
 {
    d=readdir(dr);
     while(d != NULL)
     {
         
          string filename =d->d_name;
          
          
          
          if(filename != "." && filename !=".." && filename != "Downloaded")
          {
            //    if (!filename.empty() && filename[filename.length()-1] == '\n') 
            //    {
            //     filename.erase(filename.length()-1);
            //    }    

               //create a packet and add to packet_to_forward 
               //these packets are at a depth of 0 because I own these 
               Packet my_packets_d0=Packet(filename,CLIENT_UID,0,CLIENT_PORT);
               my_files.push_back(filename);
               Packet_to_forward.push(my_packets_d0);

               
          }
          d=readdir(dr);
      }
     // cout<<"starting me i have "<<Packet_to_forward.size()<<endl;
  
 }
 else
 {
     cout<<"error in directory"<<endl;
 }

   closedir(dr);
}





int find_packet(vector<Packet> FILES_TO_SEARCH,string filename,int size)
{
    for(int i=0 ; i < size ; i++ )
    {
        if(FILES_TO_SEARCH[i].filename == filename)
          return i;
    }
    return -1;
}






// bool compareFunction (string a, string b) {return a<b;} 


void print_my_files()
{
    sort(my_files.begin(),my_files.end(),compareFunction);
    for(auto itr=my_files.begin();itr!=my_files.end();itr++) cout<<*itr<<"\n";
}


void print_connection_info()
{
    for(int i=0;i<IMMEDIATE_NEIGHBOUR.size();i++)
    {
        int id=IMMEDIATE_NEIGHBOUR[i].first;
        int port=IMMEDIATE_NEIGHBOUR[i].second;
        int uid=neigh_uid_info[port];

        cout<<"Connected to "<<id<<" with unique-ID "<<uid<<" on port "<<port<<endl;
    }
}

int main(int argc, char const *argv[])
{
    
    string CONFIGURATION_FILE = argv[1];
    string DIRECTORY_PATH = argv[2];
    
    int CLIENT_ID,CLIENT_PORT,CLIENT_UID;
    
    getdetails(CONFIGURATION_FILE,DIRECTORY_PATH,CLIENT_ID,CLIENT_UID,
            CLIENT_PORT,number_of_neighbours,IMMEDIATE_NEIGHBOUR,Packet_to_forward,FILES_TO_SEARCH,my_files
            ,neigh_uid_info);
    print_my_files();
    //after this everything is populated

    //set up listening socket
    listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd == -1)
    {
        perror("SETTING UP LISTENING SOCKET FAILED!!");
    }

    struct sockaddr_in listening_addr;
    listening_addr.sin_family = AF_INET;
    listening_addr.sin_addr.s_addr = INADDR_ANY;
    listening_addr.sin_port = htons(CLIENT_PORT);

    fcntl(listen_fd,F_SETFL,O_NONBLOCK);
     if (bind(listen_fd, (struct sockaddr *)&listening_addr, sizeof(listening_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
   
    if (listen(listen_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }  

    
    

    //get_connection_info();
    
    thread reciever_thread(receive_from_everyone);

    sleep(10);
    thread forwarding_thread(forward_to_neighbour);
    
    sort(FILES_TO_SEARCH.begin(),FILES_TO_SEARCH.end(),comparePacketFunction);
    
    forwarding_thread.join();
    print_connection_info();
    //print_needed_file_info_no_hash(FILES_TO_SEARCH);
    fflush(stdout);
    reciever_thread.join();
    
    


}


//assuming i am in listening state with socket listening_fd
void receive_from_everyone()
{
    //dirty work
    
   
    int valread;
    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(listen_fd, &current_sockets);
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int k = 0;

     while (1)
    {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == listen_fd)
                {
                    int recieve_socket;
                    
                    if ((recieve_socket = accept(listen_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(recieve_socket, &current_sockets);
                }
                else
                {
                    // now deal with the recieved packet information
                    char rec_buffer[2000];
                    while((recv(i,rec_buffer,sizeof(rec_buffer),0) > 0))
                    {
                            string packet_info=string(rec_buffer);
                            vector<string> info=split_string(packet_info);
                            //cout<<packet_info<<"\n";
                            string filename=info[0];
                            int owner_uid=stoi(info[2]);
                            
                            int depth=stoi(info[4]);
                            depth++;
                            int from_port=stoi(info[7]);
                            if(neigh_uid_info.find(from_port)!=neigh_uid_info.end())
                               neigh_uid_info[from_port]=owner_uid;
                            //cout<<filename<<" "<<owner_uid<<" ye mili \n";
                            //check if i want this file and update if the new owner is more apt
                            //cout<<number_of_neighbours<<" neighbour\n";
                            int found=find_packet(FILES_TO_SEARCH,filename,FILES_TO_SEARCH.size());//cout<<filename<<" ye dhoondni thi "<<found<<endl;
                            if(found!=-1)
                            {
                                //i wanted this file
                                //cout<<filename<<" "<<owner_uid<<" "<<depth<<" "<<from_port<<"\n";
                                int prev_owner_uid=FILES_TO_SEARCH[found].uid;
                                if( prev_owner_uid == 0 || prev_owner_uid > owner_uid )
                                 {
                                     FILES_TO_SEARCH[found].uid  = owner_uid;
                                     FILES_TO_SEARCH[found].depth = depth;

                                 }
                            }
                         
                    }
                    FD_CLR(i, &current_sockets);
                    memset(rec_buffer,0,2000); //refreshing for next round
                }
            }
        }
        if (k == (FD_SETSIZE * 2))
            break;

                

     }

          
          
}
            






void forward_to_neighbour( )
{
        struct sockaddr_in neighbour;
        int len = sizeof(struct sockaddr_in);
        neighbour.sin_family=AF_INET;
        neighbour.sin_addr.s_addr=INADDR_ANY;
         auto start = chrono::steady_clock::now();
         
        while(true)
        {
            //cout<<"looking for info\n";
           auto curr=chrono::steady_clock::now();
           if(chrono::duration_cast<chrono::seconds>(curr - start).count()>10)
           {
               break;
           }
           
            while(!Packet_to_forward.empty())
            {
               start=chrono::steady_clock::now(); 
               Packet to_forward=Packet_to_forward.front();
               //cout<<"FN: "<<to_forward.filename<<" UID: "<<to_forward.uid<<" DEPTH "<<to_forward.depth<<" PORT "<<to_forward.from_port<<endl;
               
               string file=to_forward.filename;//cout<<file<<" ";
               int uid=to_forward.uid;
               int depth=to_forward.depth;
               int from_port=to_forward.from_port;

               char buffer[2000]; 
               sprintf(buffer,"%s from %d at %d from port %d" ,file.c_str(),uid,depth,from_port);
               
               //send this to all neighbours
              
               for(int i=0;i<number_of_neighbours;i++)
               {
                 //  cout<<file<<" "<<"from"<<from_port<<"to"<<IMMEDIATE_NEIGHBOUR[i].second<<"\n";
                  
                        int sock=socket(AF_INET,SOCK_STREAM,0);
                        neighbour.sin_port=htons(IMMEDIATE_NEIGHBOUR[i].second);

                        if(connect(sock,(struct sockaddr*)&neighbour,(socklen_t)len)<0)
                            cout<<"ERROR WHILE TRYING TO REACH NEIGHBOUR\n";

                        send(sock,buffer,sizeof(buffer),0) ;

                        close(sock);
                   
                              

               }

               Packet_to_forward.pop();
            }

        }
       
}




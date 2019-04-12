#include <stdio.h>
#include <errno.h>
#include <cstring>
#include <arpa/inet.h>
#include <string>
#include <libiptc/libiptc.h>
#include <curl/curl.h>

#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h> 
/********************************************************************************/
/*                                                                              */
/*                                                                              */
/*                                                                              */
/********************************************************************************/
class NetworkManager {
  private:
    static const int PORT_NO = 0; 
    static const int RECV_TIMEOUT = 1;
    static const int PING_PKT_S = 64;
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
      return written;
    }
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    // https://curl.haxx.se/libcurl/c/example.html
    // https://curl.haxx.se/libcurl/c/url2file.html
    void download(const std::string&& url, const std::string&& out_file) 
    {
      curl_global_init(CURL_GLOBAL_ALL);

      /* init the curl session */ 
      CURL *curl_handle = curl_easy_init();

      /* set URL to get here */ 
      curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    
      /* Switch on full protocol/debug output while testing */ 
      curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    
      /* disable progress meter, set to 0L to enable and disable debug output */ 
      curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    
      /* send all data to this function  */ 
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

      /* open the file */ 
      FILE *pagefile = fopen(out_file.c_str(), "wb");
      if(pagefile) {
        /* write the page body to this file handle */ 
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    
        /* get it! */ 
        curl_easy_perform(curl_handle);
    
        /* close the header file */ 
        fclose(pagefile);
      }

      /* cleanup curl stuff */ 
      curl_easy_cleanup(curl_handle);
    
      curl_global_cleanup();
    }

    struct ping_pkt 
    { 
        struct icmphdr hdr; 
        char msg[PING_PKT_S-sizeof(struct icmphdr)]; 
    }; 
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    // calculating the checksum
    unsigned short checksum(void *b, int len) 
    {    
        unsigned short *buf = static_cast<short unsigned int*>(b); 
        unsigned int sum = 0; 
        unsigned short result = 0; 
      
        for ( sum = 0; len > 1; len -= 2 ) {
          sum += *buf++; 
        }
        if ( len == 1 ) {
          sum += *(unsigned char*)buf; 
        }
        sum = (sum >> 16) + (sum & 0xFFFF); 
        sum += (sum >> 16); 
        result = ~sum; 
        return result; 
    }
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    // performs a DNS lookup  
    char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con) 
    { 
      printf("\nResolving DNS..\n"); 
      struct hostent *host_entity; 
      char *ip=(char*)malloc(NI_MAXHOST*sizeof(char)); 
      int i; 
    
      if ((host_entity = gethostbyname(addr_host)) == NULL) 
      { 
          // No ip found for hostname 
          return NULL; 
      } 
        
      //filling up address structure 
      strcpy(ip, inet_ntoa(*(struct in_addr *) 
                            host_entity->h_addr)); 
    
      (*addr_con).sin_family = host_entity->h_addrtype; 
      (*addr_con).sin_port = htons (PORT_NO); 
      (*addr_con).sin_addr.s_addr  = *(long*)host_entity->h_addr; 
    
      return ip;          
    }
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    // resolves the reverse lookup of the hostname 
    char* reverse_dns_lookup(char *ip_addr) 
    { 
        struct sockaddr_in temp_addr;     
        socklen_t len; 
        char buf[NI_MAXHOST], *ret_buf; 
      
        temp_addr.sin_family = AF_INET; 
        temp_addr.sin_addr.s_addr = inet_addr(ip_addr); 
        len = sizeof(struct sockaddr_in); 
      
        if (getnameinfo((struct sockaddr *) &temp_addr, len, buf,  
                        sizeof(buf), NULL, 0, NI_NAMEREQD))  
        { 
            printf("Could not resolve reverse lookup of hostname\n"); 
            return NULL; 
        } 
        ret_buf = (char*)malloc((strlen(buf) +1)*sizeof(char) ); 
        strcpy(ret_buf, buf); 
        return ret_buf; 
    } 
    /********************************************************************************/
    /*                                                                              */
    /*                                                                              */
    /*                                                                              */
    /********************************************************************************/
    // https://www.geeksforgeeks.org/ping-in-c/
    void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, 
                    char *ping_dom, char *ping_ip, char *rev_host) 
    { 
      int ttl_val=64, msg_count=0, i, addr_len, flag=1, msg_received_count=0; 

      struct ping_pkt pckt; 
      struct sockaddr_in r_addr; 
      struct timespec time_start, time_end, tfs, tfe; 
      long double rtt_msec=0, total_msec=0; 
      struct timeval tv_out; 
      tv_out.tv_sec = RECV_TIMEOUT; 
      tv_out.tv_usec = 0; 
    
      clock_gettime(CLOCK_MONOTONIC, &tfs); 

      // set socket options at ip to TTL and value to 64, 
      // change to what you want by setting ttl_val 
      if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,  
                &ttl_val, sizeof(ttl_val)) != 0) 
      { 
          printf("\nSetting socket options to TTL failed!\n"); 
          return; 
      } 
      else
      { 
          printf("\nSocket set to TTL..\n"); 
      } 
    
      // setting timeout of recv setting 
      setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out); 
    
      { 
          // flag is whether packet was sent or not 
          flag=1; 
        
          //filling packet 
          bzero(&pckt, sizeof(pckt)); 
            
          pckt.hdr.type = ICMP_ECHO; 
          pckt.hdr.un.echo.id = getpid(); 
            
          for ( i = 0; i < sizeof(pckt.msg)-1; i++ ) 
              pckt.msg[i] = i+'0'; 
            
          pckt.msg[i] = 0; 
          pckt.hdr.un.echo.sequence = msg_count++; 
          pckt.hdr.checksum = checksum(&pckt, sizeof(pckt)); 
    
          usleep(PING_SLEEP_RATE); 
    
          //send packet 
          clock_gettime(CLOCK_MONOTONIC, &time_start); 
          if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0,  
            (struct sockaddr*) ping_addr,  
              sizeof(*ping_addr)) <= 0) 
          { 
              printf("\nPacket Sending Failed!\n"); 
              flag=0; 
          } 
    
          //receive packet 
          addr_len=sizeof(r_addr); 
    
          if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0,  
              (struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count>1)  
          { 
              printf("\nPacket receive failed!\n"); 
          }
          else
          { 
              clock_gettime(CLOCK_MONOTONIC, &time_end); 
              double timeElapsed = ((double)(time_end.tv_nsec -  time_start.tv_nsec))/1000000.0;
              rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed; 
              // if packet was not sent, don't receive 
              if(flag) 
              { 
                  if(!(pckt.hdr.type ==69 && pckt.hdr.code==0))  
                  { 
                      printf("Error..Packet received with ICMP type %d code %d\n", pckt.hdr.type, pckt.hdr.code); 
                  } 
                  else
                  { 
                      printf("%d bytes from %s (h: %s) (%s) msg_seq=%d ttl=%d  rtt = %Lf ms.\n",  
                            PING_PKT_S, ping_dom, rev_host,  
                            ping_ip, msg_count, 
                            ttl_val, rtt_msec); 
                      msg_received_count++; 
                  } 
              } 
          }     
      } 
      clock_gettime(CLOCK_MONOTONIC, &tfe); 
      double timeElapsed = ((double)(tfe.tv_nsec -  
                            tfs.tv_nsec))/1000000.0; 
        
      total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+  
                            timeElapsed 
                      
      printf("\n===%s ping statistics===\n", ping_ip); 
      printf("\n%d packets sent, %d packets received, %f percent 
            packet loss. Total time: %Lf ms.\n\n",  
            msg_count, msg_received_count, 
            ((msg_count - msg_received_count)/msg_count) * 100.0, 
            total_msec);  
    }

  public:
    void ping(const std::string&& host) {
      int sockfd; 
      char *ip_addr, *reverse_hostname; 
      struct sockaddr_in addr_con; 
      int addrlen = sizeof(addr_con); 
      char net_buf[NI_MAXHOST]; 

      ip_addr = dns_lookup(const_cast<char*>(host.c_str()), &addr_con); 
      if(ip_addr==NULL) 
      { 
          printf("\nDNS lookup failed! Could not resolve hostname!\n"); 
          return; 
      } 

      reverse_hostname = reverse_dns_lookup(ip_addr); 
      printf("\nTrying to connect to '%s' IP: %s\n",  const_cast<char*>(host.c_str()), ip_addr); 
      printf("\nReverse Lookup domain: %s",  reverse_hostname); 

      sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
      if(sockfd<0) { 
        printf("\nSocket file descriptor not received!!\n"); 
        return; 
      } 
      else {
        printf("\nSocket file descriptor %d received\n", sockfd);
      }

      //send pings continuously 
      send_ping(sockfd, &addr_con, reverse_hostname, ip_addr, argv[1]); 
    }

    void checkConnectionActive() 
    {

    }

    void checkConnectionSpeed() 
    {

    }
};
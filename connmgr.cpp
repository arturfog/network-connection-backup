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

// based on https://bani.com.br/2012/05/programmatically-managing-iptables-rules-in-c-iptc/
/********************************************************************************/
/*                                                                              */
/*                                                                              */
/*                                                                              */
/********************************************************************************/
class FirewallManager 
{
  /********************************************************************************/
  /*                                                                              */
  /*                                                                              */
  /*                                                                              */
  /********************************************************************************/
  public:
    static int delete_rule (const char *table,
                const char *chain) 
    {
      struct xtc_handle *h;
      int ret = 1;

      h = iptc_init (table);
      iptc_free (h);
      return 0;
    }

    static int insert_rule (const char *table,
                const char *chain, 
                unsigned int src,
                int inverted_src,
                unsigned int dest,
                int inverted_dst,
                const char *target)
    {
        struct
        {
          struct ipt_entry entry;
          struct xt_standard_target target;
        } entry;

        struct xtc_handle *h;
        int ret = 1;

        h = iptc_init (table);
        if (!h)
        {
          fprintf (stderr, "Could not init IPTC library: %s\n", iptc_strerror (errno));      
        } else {
          memset (&entry, 0, sizeof (entry));
          /* target */
          entry.target.target.u.user.target_size = XT_ALIGN (sizeof (struct xt_standard_target));
          strncpy (entry.target.target.u.user.name, target, sizeof (entry.target.target.u.user.name));

          /* entry */
          entry.entry.target_offset = sizeof (struct ipt_entry);
          entry.entry.next_offset = entry.entry.target_offset + entry.target.target.u.user.target_size;

          if (src)
          {
            entry.entry.ip.src.s_addr  = src;
            entry.entry.ip.smsk.s_addr = 0xFFFFFFFF;
            if (inverted_src)
              entry.entry.ip.invflags |= IPT_INV_SRCIP;
          }

          if (dest)
          {
            entry.entry.ip.dst.s_addr  = dest;
            entry.entry.ip.dmsk.s_addr = 0xFFFFFFFF;
            if (inverted_dst)
              entry.entry.ip.invflags |= IPT_INV_DSTIP;
          }

          if (!iptc_append_entry (chain, (struct ipt_entry *) &entry, h))
          {
            fprintf (stderr, "Could not insert a rule in iptables (table %s): %s\n", table, iptc_strerror (errno));
          } else {
            if (!iptc_commit (h))
            {
              fprintf (stderr, "Could not commit changes in iptables (table %s): %s\n", table, iptc_strerror (errno));
            } else {
              ret = 0;
            }
          }
          iptc_free (h);
        }

        return ret;
    }
};
/********************************************************************************/
/*                                                                              */
/*                                                                              */
/*                                                                              */
/********************************************************************************/
int main(int argc, char *argv[]) {

  unsigned int src, dst;

  inet_pton (AF_INET, "1.2.3.4", &src);
  inet_pton (AF_INET, "4.3.2.1", &dst);

  // iptables -t filter -A INPUT -s 1.2.3.4/32 ! -d 4.3.2.1/32 -j DROP
  FirewallManager::insert_rule ("filter",
              "INPUT",
              src,
              0,
              dst,
              1,
              "DROP");

  return 0;
}


/*Edited by Tomás Lagos*/

typedef struct NodeList {
  struct nodes_addr{
  		uint8_t link_local[16];
  		uint8_t global[16];
  }IPv6;					// nodes IPv6 address cache
  struct nodes_mac{
  		uint8_t addr[6];
  }mac;						// nodes mac address cache
  int type;					// 0 = nodes, 1 = gateway
  struct NodeList *next;
  struct NodeList *back;
} NodeList;

typedef struct IPv6PackageRawData
{
	struct package_addr
	{
		uint8_t addr[16];
	}IPv6_src,IPv6_dst;
	uint8_t next_h;
	uint16_t payload_length;
	uint8_t h_limit;
} IPv6PackageRawData;

typedef struct ICMP6PackageRawData
{
  uint8_t type;
  struct checksum_bytes
  {
    uint8_t byte[2];
  }checksum;
  struct target_addr
  {
    uint8_t addr[16];  
  }target;
  struct mac_addr
  {
    uint8_t addr[6];
  }mac;
}ICMP6PackageRawData;



uint16_t checksum_icmpv6(char *,int);

uint8_t *IPv6_address(uint8_t *, uint8_t *, int);

NodeList *get_info_by_IPv6(NodeList *, uint8_t *);

NodeList *get_info_by_mac(NodeList *, uint8_t *);

void add_node(NodeList **, uint8_t *, int);

char *icmp_reply(char *, uint8_t *);

char *router_solicitation(uint8_t *, char *, uint8_t*);

char *router_advertisement(char *, uint8_t *, uint8_t *);

char *neighbor_solicitation(char *, uint8_t *, uint8_t *, uint8_t * ,uint8_t *);

char *neighbor_advertisement(char *, uint8_t *, uint8_t *, uint8_t *);

char *redirect(char *, uint8_t *, uint8_t *, uint8_t *, uint8_t *);

IPv6PackageRawData *get_IPv6_data_by_raw_package(char *, IPv6PackageRawData *);

ICMP6PackageRawData *get_ICMP6_data_by_raw_payload(char *, ICMP6PackageRawData *);


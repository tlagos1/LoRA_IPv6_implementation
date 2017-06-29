
/*Edited by Tomás Lagos*/

typedef struct NodeList {
  struct nodes_addr{
  		uint8_t addr[16];
  }IPv6_link_local,IPv6_global;					// nodes IPv6 address cache
  
  struct nodes_mac{
  		uint8_t addr[6];
  }mac;						// nodes mac address cache
  
  struct nodes_eui{
      uint8_t addr[8];
  }eui; 

  int type;					// 0 = nodes, 1 = gateway
  
  struct NodeList *next;
  struct NodeList *back;
} NodeList;

typedef struct Saved_addr
{
  struct IPv6_Saved_addr
  {
    uint8_t addr[16];
  } src_addr, dst_addr;
  
  uint8_t eui[8];

  int type;

  struct Saved_addr *next;
  struct Saved_addr *back;
}Saved_addr;

typedef struct IPv6PackageRawData
{
	struct package_addr{
	uint8_t addr[16];
	}IPv6_src,IPv6_dst;
	uint8_t next_h;
	uint16_t payload_length;
	uint8_t h_limit;
} IPv6PackageRawData;

typedef struct ICMP6PackageRawData
{
  uint8_t type;
  uint8_t checksum[2];
  uint8_t target[16];
  uint8_t mac[6];

}ICMP6PackageRawData;

uint16_t checksum_icmpv6(char *,int);

uint8_t *IPv6_address(uint8_t *, uint8_t *, int);

NodeList *get_info_by_IPv6(NodeList *, uint8_t *);

NodeList *get_info_by_eui(NodeList *, uint8_t *);

NodeList *get_info_by_mac(NodeList *, uint8_t *);

NodeList *check_head(NodeList *);

NodeList *add_IPv6_node(NodeList *, uint8_t *, uint8_t *);

NodeList *add_MAC_node(NodeList *, uint8_t *, int);

NodeList *add_EUI_node(NodeList *, uint8_t *, int);

Saved_addr *clean_addr(uint8_t *, Saved_addr *);

Saved_addr *output_addr(uint8_t *, Saved_addr *, int);

Saved_addr *check_head_Saved_addr(Saved_addr *);

Saved_addr *input_addr(uint8_t *, uint8_t *, uint8_t *, Saved_addr *,int );

char *icmp_reply(char *, uint8_t *);

char *router_solicitation(uint8_t *, char *,uint8_t *);

char *router_advertisement(char *, uint8_t *, uint8_t *);

char *neighbor_solicitation(char *, uint8_t *, uint8_t *, uint8_t *);

char *neighbor_advertisement(char *, uint8_t *, uint8_t *, uint8_t *);

char *redirect(char *, uint8_t *, uint8_t *, uint8_t *, uint8_t *);

IPv6PackageRawData *get_IPv6_data_by_raw_package(char *, IPv6PackageRawData *);

ICMP6PackageRawData *get_ICMP6_data_by_raw_payload(char *, ICMP6PackageRawData *);
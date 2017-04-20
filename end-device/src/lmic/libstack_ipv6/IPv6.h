
/*Edited by Tomás Lagos*/

typedef struct NodeList {
  struct nodes_addr{
  		uint8_t addr[16];
  }IPv6;					// nodes IPv6 address cache
  struct nodes_mac{
  		uint8_t addr[6];
  }mac;						// nodes mac address cache
  int type;					// 0 = nodes, 1 = gateway
  struct NodeList *next;
  struct NodeList *back;
} NodeList;




uint16_t checksum_icmpv6(char *,int);

uint8_t *IPv6_address(uint8_t *, uint8_t *);

NodeList *get_info_by_IPv6(NodeList *, uint8_t *);

NodeList *get_info_by_mac(NodeList *, uint8_t *);

void add_node(NodeList **, uint8_t *, int);

char *icmp_reply(char *, uint8_t *, uint8_t *);

char *router_solicitation(uint8_t *, char *);

char *router_advertisement(char *, uint8_t *, uint8_t *);

char *neighbor_solicitation(char *, uint8_t *, uint8_t *, uint8_t *);

char *neighbor_advertisement(char *, uint8_t *, uint8_t *);

char *redirect(char *, uint8_t *, uint8_t *, uint8_t *, uint8_t *);


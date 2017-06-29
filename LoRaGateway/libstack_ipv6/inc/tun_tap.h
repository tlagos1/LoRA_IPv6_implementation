
/*Edited by Tomás Lagos*/

int read_n(int , char *, int );

int init_socket();

int init_tun(); 

int send_raw_package(char *, uint8_t*, uint8_t*, char *, int);

char *recive_raw_package_by_interface(int, char *);
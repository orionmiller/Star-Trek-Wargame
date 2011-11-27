
#ifndef LIB_SERVICE_H_
#define LIB_SERVICE_H_

#define SK_IP "10.13.37.40"
#define SK_PORT 4443

#define PWR_IP "10.13.37.40"
#define PWR_ERR (-1)
#define PWR_ERR_CONN (-1)
#define PWR_ERR_ALLOC (-1)
#define PWR_ERR_RECV_PWR (-1)

int get_pwr_alloc(void);

void (*print_status)(void);
void *(*request_handler)();
extern void run_tests(void);
void service_startup();
void service_shutdown();

#endif


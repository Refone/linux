#include <linux/printk.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/socket.h>
//#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#define MIG_PORT		11603  //network port for migration.
#define MODULE_NAME		"cuckoo-sender"
static struct sockaddr_in servaddr;
static struct socket *serv_sock = NULL;
static struct socket *connect_sock = NULL;
static struct task_struct *accept_worker = NULL;

int ktcp_accept_worker()
{
	int r = -1;

	printk("[CUCKOO-SENDER] accept worker fired!\n");

	r = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&connect_sock);
	if (r<0) {
			printk("[CUCKOO-SENDER] connect_sock create error.\n");
			return -1;
	}		
	
	r = serv_sock->ops->accept(serv_sock, connect_sock, 0);
	if (r<0) {
			printk("[CUCKOO-SENDER] accept error.\n");
			sock_release(connect_sock);
			return -1;
	}

	
	printk("cross.\n");

}

static int __init mig_sender_init(void)
{
		int r = -1;

		printk("[CUCKOO-SENDER] mig_sender_init invoked!\n");


		r = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &serv_sock);
		if (r<0) {
			printk("[CUCKOO-SENDER] serv_sock create error.\n");
			return -1;
		}		

		memset(&servaddr,0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(PORT);
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		r = serv_sock->ops->bind(serv_sock,
						(struct sockaddr *)&servaddr,
						sizeof (servaddr));
		if (r<0) {
			printk("[CUCKOO-SENDER] serv_sock bind error.\n");
			sock_release(serv_sock);
			return -1;
		}		

		r = serv_sock->ops->listen(serv_sock, 1);
		if (r<0) {
			printk("[CUCKOO-SENDER] serv_sock listen error.\n");
			sock_release(serv_sock);
			return -1;
		}		

		accept_worker = kthread_run((void *)ktcp_accept_worker, NULL, MODULE_NAME);

		return 0;
}
module_init(mig_sender_init)

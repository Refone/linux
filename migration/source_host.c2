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
#define ADDRESS_LEN		8
#define PAGE_LEN		4096

static int migration_processing = 0;	// when migration is processing, it is 1;
EXPORT_SYMBOL_GPL(migration_processing);

static int sender_serving = 0;			// when sender is on service, it is 1;
EXPORT_SYMBOL_GPL(sender_serving);

static struct sockaddr_in servaddr;
static struct socket *serv_sock = NULL;
static struct task_struct *accept_worker = NULL;

void fetch_page(char* address, char* page_content) {
	int i;
	//memset(page_content, 0, PAGE_LEN);
    for (i=0; i<PAGE_LEN; i+=8) {
		strncpy(&content[i], address, 8);
	}
}

int read_response(struct socket *sock, char *str)
{
    struct msghdr msg;
    struct iov_iter iter;
    struct iovec iov;
    int len = 0;

    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    iov.iov_base = str;
    iov.iov_len = ADDRESS_LEN;

    iter.type = 0;
    iter.iov_offset = 0;
    iter.count = ADDRESS_LEN;
    iter.iov = &iov;
    iter.nr_segs = 1;

    msg.msg_name     = 0;
    msg.msg_namelen  = 0;
    //msg.msg_iov      = &iov;
    //msg.msg_iovlen   = 1;
    msg.msg_iter     = iter;
    msg.msg_control  = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags    = 0;

    len = sock_recvmsg(sock, &msg, 0);

    set_fs(oldfs);

	printk("[CUCKOO-SENDER] %d bytes received.\n", len);

    return len;
}

void send_reply(struct socket *sock, char *buf)
{
    struct msghdr msg;
    struct iovec iov;
    //struct iov_iter iter;
    //int len, written = 0, left = length;
    int len = 0;
    mm_segment_t oldmm;

    //copy_to_iter(buf, length, &iter);
	
    iov.iov_base = buf;
    iov.iov_len = strlen(buf);

    msg.msg_name     = 0;
    msg.msg_namelen  = 0;
    //msg.msg_iov      = &iov;
    //msg.msg_iovlen   = 1;
    msg.msg_iter.type = 0;
    msg.msg_iter.count = strlen(buf);
    msg.msg_iter.iov_offset = 0;
    msg.msg_iter.iov = &iov;

    //copy_to_iter(buf, length, &(msg.msg_iter));
    msg.msg_control  = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags    = MSG_DONTWAIT;

    oldmm = get_fs(); set_fs(KERNEL_DS);

    len = sock_sendmsg(sock, &msg);
    
    set_fs(oldmm);

    printk("[CUCKOO-SENDER]: %d bytes sent.\n", len);

    return len;

}

int ktcp_accept_worker()
{
	int r = -1;
	struct socket *connect_sock = NULL;
	char page_address[ADDRESS_LEN+1];
	char page_content[PAGE_LEN+1];

	printk("[CUCKOO-SENDER] accept worker fired!\n");

	r = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&connect_sock);
	if (r<0) {
			printk("[CUCKOO-SENDER] connect_sock create error.\n");
			return -1;
	}		

	for (;;) {	

		r = serv_sock->ops->accept(serv_sock, connect_sock, 0);
		if (r<0) {
			printk("[CUCKOO-SENDER] accept error.\n");
			sock_release(connect_sock);
			return -1;
		}

		//handle connection
		for(;;) {
			memset(page_address, 0, ADDRESS_LEN+1);
			read_response(connect_sock, page_address);
			memset(page_content, 0, PAGE_LEN+1);
			fetch_page(page_address, page_content);
			send_reply(connect_sock, page_content);

			if (strcmp(page_address, "00000000")==0) {
				break;
			}
		}

		printk("[CUCKOO-SENDER] accept looper.\n");
	}

	sock_release(connect_sock);
	return 0;
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

		sender_serving = 1;

		accept_worker = kthread_run((void *)ktcp_accept_worker, NULL, MODULE_NAME);

		return 0;
}
module_init(mig_sender_init)

static int __exit mig_sender_exit(void)
{
	printk("[CUCKOO-SENDER] module exit.");
	sock_release(serv_sock);
	kthread_stop(accept_worker);
}
module_exit(mig_sender_exit);

MODULE_LICENSE("GPL");

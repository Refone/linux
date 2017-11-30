#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>

#define NETWORK_PAGE_FAULT

#define MODULE_NAME        "cuckoo-receiver"
#define MIG_PORT        11603
#define ADDRESS_LEN        16
//#define PAGE_LEN		 4096
#define PAGE_LEN		 2097152

static int sock_connected = 0;
EXPORT_SYMBOL_GPL(sock_connected);
static int npf_on = 1;
EXPORT_SYMBOL_GPL(npf_on);

static struct sockaddr_in serv_addr;
static struct socket *connect_sock = NULL;

static char page_address[ADDRESS_LEN+1];
static char page_content[PAGE_LEN+1];

int send_reply(struct socket *sock, char *buf);
int read_response(struct socket *sock, char *str);
int __try_connect(void);

static uint32_t create_address(uint8_t a1,uint8_t a2,uint8_t a3,uint8_t a4)
{
		uint32_t address;

		address = 0;
		address = address | (a1 << 24);
		address = address | (a2 << 16);
		address = address | (a3 << 8);
		address = address | (a4);

		return address;
}

void request_page(unsigned long address, char* content)
{

		if(!sock_connected) {
			if(__try_connect()<0){
				printk("[CUCKOO-RECEIVER] request page failed.\n");
				return;
			}
		}    

		memset(page_address, 0, ADDRESS_LEN);
		snprintf(page_address, ADDRESS_LEN, "%lx", address);
		page_address[ADDRESS_LEN] = '\0';

		//printk("[CUCKOO-RECEIVER] page addr:%s.\n", page_address);

		send_reply(connect_sock, page_address);
		memset(page_content, 0, PAGE_LEN);
		read_response(connect_sock, page_content);

		page_content[PAGE_LEN] = '\0';
		//printk("[CUCKOO-RECEIVER] page content:%s.\n", page_content);
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
		iov.iov_len = PAGE_LEN;

		iter.type = 0;
		iter.iov_offset = 0;
		iter.count = PAGE_LEN;
		iter.iov = &iov;
		iter.nr_segs = 1;

		msg.msg_name     = 0;
		msg.msg_namelen  = 0;
		//msg.msg_iov      = &iov;
		//msg.msg_iovlen   = 1;
		msg.msg_iter     = iter;
		msg.msg_control  = NULL;
		msg.msg_controllen = 0;
		//msg.msg_flags    = 0;
		msg.msg_flags    = MSG_WAITALL;

		//for(len=0;len==0;len = sock_recvmsg(sock, &msg, 0));
		for(len=0;len==0;len = sock_recvmsg(sock, &msg, MSG_WAITALL));

		set_fs(oldfs);

		printk("[CUCKOO-RECEIVER] %d bytes received.\n", len);

		return len;
}


int send_reply(struct socket *sock, char *buf)
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
		//msg.msg_flags    = MSG_DONTWAIT;
		msg.msg_flags    = MSG_WAITALL;

		oldmm = get_fs(); set_fs(KERNEL_DS);

		len = sock_sendmsg(sock, &msg);

		set_fs(oldmm);

		//printk("[CUCKOO-RECEIVER]: %d bytes sent.\n", len);

		return len;

}

int __try_connect(void)
{
	int r = -1;
	r = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &connect_sock);
	if (r < 0) {
		printk("[CUCKOO-RECEIVER]: connect sock create failed.\n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(MIG_PORT);
	serv_addr.sin_addr.s_addr = htonl(create_address(192, 168, 10, 70));

	r = connect_sock->ops->connect(connect_sock,
			(struct sockaddr *) &serv_addr,
			sizeof(serv_addr), O_RDWR);

	if (r==0) {
		printk("[CUCKOO-RECEIVER]: connect to server.\n");
	} else {
		printk("[CUCKOO-RECEIVER]: connect failed.\n");
		return -1;
	}

	sock_connected = 1;	
	return 0;
}
EXPORT_SYMBOL_GPL(__try_connect);

static int __init mig_receiver_init(void)
{
		printk("[CUCKOO-RECEIVER]: module init.\n");
		sock_connected = 0;

		//request_page(0x60000000, page_content);
		return 0;
}

static void __exit mig_receiver_exit(void)
{
		sock_release(connect_sock);
		sock_connected = 0;
		printk("[CUCKOO-RECEIVER] module exit.\n");
}

module_init(mig_receiver_init);
module_exit(mig_receiver_exit);

MODULE_LICENSE("GPL"); 

#ifndef NETWORK_PAGE_FAULT
void __request_page(unsigned long address)
{
		printk("[LRF] migration LOCAL request page: %lx\n", address);
		return;
}
EXPORT_SYMBOL_GPL(__request_page);
#else
void __request_page(unsigned long address)
{
		if (npf_on) {
			request_page(address, page_content);
			printk("[LRF] migration NETWORK request page: %lx\n", address);
		} else {
			printk("[LRF] migration LOCAL request page: %lx\n", address);
		}

		return;
}
EXPORT_SYMBOL_GPL(__request_page);
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <net/rtnetlink.h>
#include <linux/wireless.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/notifier.h>
#include <linux/if_ether.h>
#include <uapi/linux/if_arp.h>
#include <linux/if_arp.h>
#include <linux/inetdevice.h>
#include <linux/ip.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../fs/proc/internal.h>
#include <linux/udp.h>
#include <uapi/linux/udp.h>

//add because vwlan_cfg80211_t structure
#include <net/cfg80211.h>
#include <linux/nl80211.h>

#include <net/rtl/rtl_vwlan.h>
#include <net/rtl/rtl_vwlan_debug.h>

//-- device table -----------------------------------------------------------------
static char REAL_DEV[IFNAMSIZ] = "eth0";
static char RX_REG_DEV[IFNAMSIZ] = "eth0";

static struct net_device *base_dev = NULL;
//static 
int dev_num;
static int vwlan_running = 0;

vwlan_dev_config_t vwlan_dev_table[] = {
	/* virtual interface for rpc */
	{"vwlan",	0, 0, NULL, { 0x00, 0x12, 0x34, 0x56, 0xfa, 0x3e }, NULL, 0, IFACE_VWLAN},

	/* virtual interface for wlan0 */
	{"wlan0",	1, 1, NULL, { 0x00, 0xe0, 0x4c, 0x81, 0x86, 0x87 }, NULL, 0, IFACE_WLAN0_0},
  #if 0
	{"wlan0-1", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x01 }, NULL, 0, IFACE_WLAN0_1},
	{"wlan0-2", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x02 }, NULL, 0, IFACE_WLAN0_2},
	{"wlan0-3", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x03 }, NULL, 0, IFACE_WLAN0_3},
	{"wlan0-4", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x04 }, NULL, 0, IFACE_WLAN0_4},
	{"wlan0-5", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x05 }, NULL, 0, IFACE_WLAN0_5},
	{"wlan0-6", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x06 }, NULL, 0, IFACE_WLAN0_6},
	{"wlan0-7", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x07 }, NULL, 0, IFACE_WLAN0_7},
	{"wlan0-8", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x08 }, NULL, 0, IFACE_WLAN0_8},
	{"wlan0-9", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf1, 0x09 }, NULL, 0, IFACE_WLAN0_9},
  #endif

	/* virtual interface for wlan1 */
	{"wlan1",	1, 1, NULL, { 0x00, 0xe0, 0x4c, 0x81, 0x86, 0x86 }, NULL, 0, IFACE_WLAN1_0},	
  #if 0
	{"wlan1-1", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x01 }, NULL, 0, IFACE_WLAN1_1},
	{"wlan1-2", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x02 }, NULL, 0, IFACE_WLAN1_2},
	{"wlan1-3", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x03 }, NULL, 0, IFACE_WLAN1_3},
	{"wlan1-4", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x04 }, NULL, 0, IFACE_WLAN1_4},
	{"wlan1-5", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x05 }, NULL, 0, IFACE_WLAN1_5},
	{"wlan1-6", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x06 }, NULL, 0, IFACE_WLAN1_6},
	{"wlan1-7", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x07 }, NULL, 0, IFACE_WLAN1_7},
	{"wlan1-8", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x08 }, NULL, 0, IFACE_WLAN1_8},
	{"wlan1-9", 0, 1, NULL, { 0x00, 0x12, 0x34, 0x56, 0xf2, 0x09 }, NULL, 0, IFACE_WLAN1_9},
  #endif
};

//-- socket for RPC ---------------------------------------------------------------
#define MODULE_NAME 	"rtl_vwlan"
#define DRV_VERSION 	"0.1.0"

static DEFINE_MUTEX(pm_mutex);

char buffer[MAXLENGTH];
static struct kthread_t *kthread = NULL;
static int socket_created = 0;

static struct socket *sock;
static struct socket *sock_send;
static struct sockaddr_in addr;
static struct sockaddr_in addr_send;

atomic_t base_dev_running;
atomic_t vrit_dev_running;

int is_rpc_ready(void)
{
	bdbg(LV_0324_CHK_BASE_DEV, 
		"base_dev_running[%d] && vrit_dev_running[%d] && socket_created[%d]\n", 
		atomic_read(&base_dev_running), 
		atomic_read(&vrit_dev_running), 
		socket_created);

	if (atomic_read(&base_dev_running) && 
		atomic_read(&vrit_dev_running) && 
		socket_created) {
		return 0;/* rpc ok */
	}

	return 1;
}

void vwlan_socket_create(void)
{
	int err, val = 1;

	if (socket_created == 0) {
		if (((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0)
				||
			 ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_send)) < 0 )) {
			bdbg_err("%s: Could not create a datagram socket, error = %d\n", 
				MODULE_NAME, -ENXIO);

			socket_created = 0;
			return;
		}

		memset(&addr, 0, sizeof(struct sockaddr_in));
		memset(&addr_send, 0, sizeof(struct sockaddr_in));
		addr.sin_family 	 = AF_INET;
		addr_send.sin_family = AF_INET;

		addr.sin_addr.s_addr	  = htonl(INADDR_MASTER);
		addr_send.sin_addr.s_addr = htonl(INADDR_SLAVE);

		addr.sin_port	   = htons(DEFAULT_PORT);
		addr_send.sin_port = htons(DEFAULT_PORT);


		/* SET SOCKET REUSE Address */
		kernel_setsockopt(sock, SOL_SOCKET, 
			SO_REUSEADDR, (char *)&val, sizeof(val));
	
		if ((err = sock->ops->bind(sock, (struct sockaddr *)&addr, 	sizeof(struct sockaddr))) < 0) {
			bdbg_err("%s: Could not bind or connect to socket, error = %d\n", 
				MODULE_NAME, -err);

			sock_release(sock);
			sock_release(sock_send);
			socket_created = 0;
			return;
		}
		else {
			socket_created = 1;
			bdbg(LV_TRACE, "socket created!!\n");
		}
	}
}

int ksocket_send(struct socket *sock, struct sockaddr_in *addr, 
				unsigned char *buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL)
		return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);

	return size;
}

int ksocket_receive(struct socket* sock, struct sockaddr_in* addr, 
				unsigned char* buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL) return 0;
	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(sock,&msg,len,msg.msg_flags);
	set_fs(oldfs);
	return size;
}

static void process_signal( vwlan_sig_packet_t *vp)
{
	pid_t pid;

	if(vp->ret == 2){
		memcpy(&pid, vp->data, sizeof(pid_t));
		bdbg(LV_TRACE, "send signal to pid (%d)\n", pid);
		kill_pid(find_get_pid(pid), SIGIO, 1);
	}
	else{
		bdbg(LV_TRACE, "do nothing!\n");
	}
}

static void ksocket_start(void)
{
	int size, err;
	char buf[sizeof(vwlan_sig_packet_t)];
	vwlan_sig_packet_t *vp;
	vp = (vwlan_sig_packet_t *)buf;

	rtnl_lock();
	kthread->running = 1;
	current->flags |= PF_NOFREEZE;
	allow_signal(SIGKILL);
	rtnl_unlock();

	if ( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &kthread->sock)) < 0)	{
		bdbg_err("%s: Could not create a datagram socket, error = %d\n", 
			MODULE_NAME, -ENXIO);
		goto out;
	}

	memset(&kthread->addr, 0, sizeof(struct sockaddr_in));
	kthread->addr.sin_family = AF_INET;

	kthread->addr.sin_addr.s_addr = htonl(INADDR_MASTER);
	kthread->addr.sin_port = htons(SIG_PORT);

	if ( (err = kthread->sock->ops->bind(kthread->sock, (struct sockaddr *)&kthread->addr, sizeof(struct sockaddr) ) ) < 0) {
		bdbg_err("%s: Could not bind or connect to socket, errrro = %d\n", 
			MODULE_NAME, -err);
		goto close_and_out;
	}

	for (;;) {
		bdbg(LV_0314_RX_DEV, "\n");
		memset(&buf, 0, sizeof(vwlan_sig_packet_t));
		size = ksocket_receive(kthread->sock, 
			&kthread->addr, buf, sizeof(vwlan_sig_packet_t));

		if (signal_pending(current))
			break;

		if (size < 0) {
			bdbg_err("%s: error getting datagram, sock_recvmsg error = %d\n", 
				MODULE_NAME, size);
		}
		else {
			bdbg_err("%s: received %d bytes\n", MODULE_NAME, size);
			process_signal(vp);
		}
	}

close_and_out:
	bdbg(LV_0314_RX_DEV, "close_and_out\n");
	sock_release(kthread->sock);
	kthread->sock = NULL;

out:
	bdbg(LV_0314_RX_DEV, "out\n");
	kthread->thread = NULL;
	kthread->running = 0;
}

static void signal_socket_start(void)
{
	/* start kernel thread */
	kthread->thread = kthread_run((void *)ksocket_start, NULL, MODULE_NAME);
	if (IS_ERR(kthread->thread)) {
		bdbg_err("%s: unable to start kernel thread\n", MODULE_NAME);
		kfree(kthread);
		kthread = NULL;
	}
}

static void signal_socket_end(void)
{
	int err;

	if(kthread->thread == NULL) {
		bdbg_err("%s: no kernel thread to kill\n", MODULE_NAME);
	}
	else {
		rtnl_lock();
		err = kill_pid(find_get_pid(kthread->thread->pid), SIGKILL, 1);
		rtnl_unlock();

		if (err < 0) {
			bdbg_err("%s: unknown error %d while trying to terminate kernel thread\n", 
				MODULE_NAME, -err);
		}
		else {
			while (kthread->running == 1)
				msleep(10);
			bdbg_err("%s: succesfully killed kernel thread!\n", 
				MODULE_NAME);
		}
	}

	if (kthread->sock != NULL) {
		sock_release(kthread->sock);
		kthread->sock = NULL;
	}
}

//-- rx handler -------------------------------------------------------------------
static inline int IS_MCAST(unsigned char *da)
{
	if ((*da) & 0x01)
		return 1;
	else
		return 0;
}

struct dhcpMessage {
	unsigned char op;
	unsigned char htype;
	unsigned char hlen;
	unsigned char hops;
	unsigned int xid;
	unsigned short secs;
	unsigned short flags;
	unsigned int ciaddr;
	unsigned int yiaddr;
	unsigned int siaddr;
	unsigned int giaddr;
	unsigned char  chaddr[16];
	unsigned char sname[64];
	unsigned char file[128];
	unsigned int cookie;
	unsigned char options[308]; /* 312 - cookie */
};

static inline int dispatch_arp(struct sk_buff *skb)
{
	struct arp_packet *etharp = NULL;
	int i;
	
	for (i = 0; i < dev_num; i++) {
		if (vwlan_dev_table[i].ipaddr) {
			etharp = (struct arp_packet *)skb_mac_header(skb);
	
			/* arp target ip == interface IP */
			if (etharp && vwlan_dev_table[i].ipaddr == etharp->ar_tip) {
				bdbg(LV_0321_RXHANDLER, 
					"arp target ip == %s interface ip\n", 
					vwlan_dev_table[i].dev->name);
	
				skb->dev = vwlan_dev_table[i].dev;
				return RX_HANDLER_PASS;
			}
		}
	}
	return RX_HANDLER_PASS;
}

static inline int dispatch_dhcp(struct sk_buff *skb)
{
	struct udphdr *udph = NULL;
	struct dhcpMessage *dhcp = NULL;
	int i;

	if (ip_hdr(skb)->protocol == IPPROTO_UDP) {
		udph = (struct udphdr *)((unsigned long)ip_hdr(skb) + (ip_hdr(skb)->ihl << 2));
		/*bdbg(LV_0328_DHCP, 
			"sport=0x%x, dport=0x%x\n", 
			udph->source, udph->dest);*/

		if (udph->source == 67 && udph->dest == 68) {
			dhcp = (struct dhcpMessage *)((unsigned long)udph + sizeof(struct udphdr));

			for (i = 0; i < dev_num; i++) {
				if (memcmp(dhcp->chaddr, vwlan_dev_table[i].dev->dev_addr, ETH_ALEN) == 0) {
					bdbg(LV_0328_DHCP, 
						"chaddr == %s interface mac\n", 
						vwlan_dev_table[i].dev->name);

					skb->dev = vwlan_dev_table[i].dev;
					return RX_HANDLER_PASS;
				}
			}
		}
	}
	return RX_HANDLER_PASS;
}

struct rpchdr asm_pkt;
static inline int dispatch_rpc(struct sk_buff *skb)
{
	extern int event_indicate_test(unsigned char *mac, int event);
	struct rpchdr *rpc = (struct rpchdr *)eth_hdr(skb);
	struct rpchdr *rpc_asm = &asm_pkt;

	if (rpc->type == RTL_RPC_CFG) {
		int hdr_sz = sizeof(struct rpchdr)-sizeof(vwlan_cfg80211_event_ind_t);
		__be16 offset = rpc->offset;

		bdbg(LV_TRACE, "rpc->id=%d, rpc->order=%d, rpc->tot_len=%d, rpc->offset=%d, rpc->mf=%d\n", rpc->id, rpc->order, rpc->tot_len, rpc->offset, rpc->mf);
		memcpy(&*(rpc_asm->msg.cfg.dev_name+offset), (void *)&rpc->msg.cfg, skb->len-hdr_sz+ETH_HLEN);
		bdbg(LV_TRACE, "skb->len=%d, cp sz=%d\n", skb->len, skb->len-hdr_sz+ETH_HLEN);

		if (rpc->mf == 0) {
		#if defined(TEST_INDICATE_EVT_IOCTL)
			bdbg(LV_TRACE, "[dispatch_rpc] test\n\n");
			vwlan_cfg80211_indicate_event_test(&rpc_asm->msg.cfg);
		#else
			extern void vwlan_cfg80211_indicate_event(vwlan_cfg80211_event_ind_t *cfg);
			bdbg(LV_TRACE, "[dispatch_rpc] cmd[%d],dev[%s]\n", rpc->msg.cfg.cmd, rpc->msg.cfg.dev_name);
			vwlan_cfg80211_indicate_event(&rpc_asm->msg.cfg);
		#endif
			bdbg(LV_TRACE, "[dispatch_rpc] Done\n\n");
		}

		kfree_skb(skb);
		return RX_HANDLER_CONSUMED;
	}
	else if (rpc->type == RTL_RPC_IDX) {
		/* assign to right device */
		return RX_HANDLER_PASS;
	}

	kfree_skb(skb);
	return RX_HANDLER_CONSUMED;
}

static inline int dispatch_unicast(struct sk_buff *skb)
{
	int i;

	for (i = 0; i < dev_num; i++) {
		if (vwlan_dev_table[i].dev && 
				memcmp(eth_hdr(skb)->h_dest, vwlan_dev_table[i].dev->dev_addr, ETH_ALEN) == 0) {
			bdbg(LV_0321_RXHANDLER, 
				"packet da == %s interface mac\n", 
				vwlan_dev_table[i].dev->name);
	
			skb->dev = vwlan_dev_table[i].dev;
			skb->pkt_type = PACKET_HOST;

	#if 0//(BDBG_LEVEL >= LV_0321_RXHANDLER)
			dbg_show_packet_type(skb, __FUNCTION__, __LINE__);
	#endif
			return RX_HANDLER_PASS;
		}
	}
	return RX_HANDLER_PASS;
}

rx_handler_result_t vwlan_rx_dispatch(struct sk_buff **pskb)
{
	int ret = RX_HANDLER_PASS; /* don't handle it */
	struct sk_buff *skb = *pskb;
  #if 0//(BDBG_LEVEL >= LV_DEBUG)
	int show = 0;
  #endif

	if (IS_MCAST(eth_hdr(skb)->h_dest)) {
		if (eth_hdr(skb)->h_proto == htons(ETH_P_ARP)) {
			ret = dispatch_arp(skb);
		}
		else if (eth_hdr(skb)->h_proto == htons(ETH_P_IP)) {
			ret = dispatch_dhcp(skb);
		}
		else if (eth_hdr(skb)->h_proto == htons(ETH_P_RTL_RPC)) {
			ret = dispatch_rpc(skb);
		}
	}
	else {
		ret = dispatch_unicast(skb);
	}

//dispatch_out:
#if 0//(BDBG_LEVEL >= LV_DEBUG)
	dbg_show_dispatch_result(skb, (char *)__FUNCTION__, __LINE__, show, ret);
#endif
	return ret;
}

int vwlan_rx_handler_reg(struct net_device *vwlan_dev)
{
	struct net_device* dev = NULL;
	int err = 0; 
	dev = dev_get_by_name(&init_net, RX_REG_DEV);
	bdbg(LV_0321_RXHANDLER, "reg on %s, %s\n", RX_REG_DEV, dev->name);	
	if (dev) {
		bdbg(LV_0314_RX_DEV, "\n");
		err = netdev_rx_handler_register(dev, vwlan_rx_dispatch, vwlan_dev);
		if (err) {
			bdbg(LV_0314_RX_DEV, "\n");
			return 1;
		}
	}
	else {
		bdbg(LV_0314_RX_DEV, "\n");
		return 1;
	}

	return 0;
 }

void vwlan_rx_handler_unreg(void)
{
	struct net_device* dev = NULL;
	dev = dev_get_by_name(&init_net, RX_REG_DEV);
	if (dev) {
		bdbg(LV_0314_RX_DEV, "vwlan_rx_handler_unreg, ok\n");
		netdev_rx_handler_unregister(dev);
	}
	else {
		bdbg(LV_0314_RX_DEV, "vwlan_rx_handler_unreg, fail\n");
	}
}

//-- vwlan proc -------------------------------------------------------------------
vwlan_proc_packet_t proc_pkt, *proc_pkt_p = &proc_pkt;

int __vwlan_proc_read(struct seq_file *seq, void *data)
{
	proc_priv_t *priv = seq->private;
	int size = -1;
	int pos = 0;

	if (is_rpc_ready())
		return 0;

	memset(&proc_pkt, '\0', sizeof(vwlan_proc_packet_t));

	bdbg(LV_DEBUG, "dev[%s], cmd[%s]\n", priv->dev->name, priv->cmd);

	proc_pkt_p->cmd_type = CMD_PROCREAD;
	sprintf(proc_pkt_p->name, "/proc/%s/%s", priv->dev->name, priv->cmd);

	ksocket_send(sock_send, &addr_send, 
		(char *)(&proc_pkt), sizeof(vwlan_proc_packet_t));
	size = ksocket_receive(sock, &addr, 
		(char *)(&proc_pkt), sizeof(vwlan_proc_packet_t));

	if(size >= 0){
		bdbg(LV_TRACE, "recieved\n");
		seq_printf(seq, "%s", proc_pkt_p->data);
	}
	else {
		bdbg_err("recieve fail!\n");
	}

	return pos;
}

int vwlan_proc_open(struct inode *inode, struct file *file)
{
	return(single_open(file, __vwlan_proc_read, PDE_DATA(inode)));
}

static int __vwlan_proc_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	if (is_rpc_ready())
		return 0;

	if (count < 2)
		return -EFAULT;

	memset(&proc_pkt, '\0', sizeof(vwlan_proc_packet_t));

	if (buffer && !copy_from_user(proc_pkt_p->data, buffer, count-1)) {
		char pathbuf[PATH_MAX], *path;	
		proc_pkt_p->cmd_type = CMD_PROCWRITE;
		path = d_path(&file->f_path, pathbuf, PATH_MAX);
		sprintf(proc_pkt_p->name, "%s", path);

		bdbg(LV_DEBUG, 
			"name[%s], data[%s]\n", 
			proc_pkt_p->name, proc_pkt_p->data);

		ksocket_send(sock_send, &addr_send, 
			(char *)(&proc_pkt), sizeof(vwlan_proc_packet_t));

		return count;
	}
	else{
		return -EFAULT;
	}
}

static ssize_t vwlan_proc_write(struct file * file, 
			const char __user * userbuf, size_t count, loff_t * off)
{
	return __vwlan_proc_write(file, userbuf,count, off);
}

static vwlan_proc_table_t vwlan_proc[] =
{
    //{ "dhcp.leases"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_all"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_rf"       , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_operation", 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_staconfig", 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_dkeytbl"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_auth"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_gkeytbl"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mib_bssdesc"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "sta_info"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "sta_keyinfo"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "sta_dbginfo"  , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "txdesc"       , 1, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
    { "rxdesc"       , 1, { .open  = vwlan_proc_open, .write = NULL, } },
    { "desc_info"    , 1, { .open  = vwlan_proc_open, .write = NULL, } },
    { "buf_info"     , 1, { .open  = vwlan_proc_open, .write = NULL, } },
    { "stats"        , 0, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
    { "mib_erp"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "cam_info"     , 1, { .open  = vwlan_proc_open, .write = NULL, } },
    { "led"          , 1, { .open  = NULL           , .write = vwlan_proc_write, } },
#if defined(WDS)
    { "mib_wds"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(RTK_BR_EXT)
    { "mib_brext"    , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(ENABLE_RTL_SKB_STATS)
    { "skb_info"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(DFS)
    { "mib_dfs"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL_8881A)
    { "mib_rf_ac"    , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
    { "mib_misc"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#if defined(WIFI_SIMPLE_CONFIG)
    { "mib_wsc"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(GBWC)
    { "mib_gbwc"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
    { "mib_11n"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#if defined(RTL_MANUAL_EDCA)
    { "mib_EDCA"     , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(CLIENT_MODE)
    { "up_flag"      , 1, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
#endif
#if defined(RTLWIFINIC_GPIO_CONTROL)
    { "gpio_ctrl"    , 1, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
#endif
#if defined(TLN_STATS)
    { "wifi_conn_stats"        , 0, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
    { "ext_wifi_conn_stats"    , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(AUTO_TEST_SUPPORT)
    { "SS_Result"    , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
#if defined(_MESH_DEBUG_) /* 802.11s output debug information */
    { "mesh_unestablish_mpinfo", 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mesh_assoc_mpinfo"      , 0, { .open  = vwlan_proc_open, .write = NULL, } },
    { "mesh_stats"   , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif  /* _MESH_DEBUG_ */
#if defined(CONFIG_RTK_VLAN_SUPPORT)
    { "mib_vlan"     , 0, { .open  = vwlan_proc_open, .write = vwlan_procwrite_mib_vlan, } },
#endif
#if 1//defined(CONFIG_RTL_WLAN_STATUS)
    { "up_event"     , 0, { .open  = vwlan_proc_open, .write = vwlan_proc_write, } },
#endif
#if defined(SUPPORT_MULTI_PROFILE)
    { "mib_ap_profile" , 0, { .open  = vwlan_proc_open, .write = NULL, } },
#endif
};

void one_device_proc(struct proc_dir_entry *proc, struct net_device *dev)
{
	int i, proc_num = ((sizeof(vwlan_proc))/(sizeof(vwlan_proc_table_t)));

	for (i = 0; i < proc_num; i++) {
		/* Don't create root_only entry for vap */
		if (strlen(dev->name)>5 && vwlan_proc[i].root_only)
			continue;

		vwlan_proc[i].priv.dev = dev;
		vwlan_proc[i].priv.cmd = (&vwlan_proc[i])->cmd;
		vwlan_proc[i].fops.read = seq_read;
		vwlan_proc[i].fops.llseek = seq_lseek;
		vwlan_proc[i].fops.release = single_release;		

		vwlan_proc[i].entry = proc_create_data(vwlan_proc[i].cmd, 0, proc,
				 &vwlan_proc[i].fops, &vwlan_proc[i].priv);

		bdbg(LV_0314_PROC, 
			"pde->data->cmd[%s], pde->data->dev[%s]\n", 
			((proc_priv_t *)(vwlan_proc[i].entry->data))->cmd, 
			((proc_priv_t *)(vwlan_proc[i].entry->data))->dev->name);
	}
}

void vwlan_create_devices_proc(void)
{
	int i;

	/* create proc dir for virtual wlan device */
	for (i = 0; i < dev_num; i++) {
		if (vwlan_dev_table[i].create_proc) {
			if (vwlan_dev_table[i].proc_dir == NULL) {
				vwlan_dev_table[i].proc_dir = 
					proc_mkdir(vwlan_dev_table[i].dev->name,NULL);
				one_device_proc(vwlan_dev_table[i].proc_dir, vwlan_dev_table[i].dev);
			}
		}
	}
}

void vwlan_remove_devices_proc(void)
{
	int i;
	int j, proc_num = ((sizeof(vwlan_proc))/(sizeof(vwlan_proc_table_t)));

	for (i = 0; i < dev_num; i++) {
		if (vwlan_dev_table[i].create_proc) {
			for (j = 0; j < proc_num; j++) {
			    remove_proc_entry(vwlan_proc[j].cmd, 
					vwlan_dev_table[i].proc_dir);
			}
			remove_proc_entry(vwlan_dev_table[i].ifname, NULL);
		}
	}
}

/*
if (eth is up && running && vwlan has ip ) create socket
*/

//-- notification chain for netdev & inet -----------------------------------------------
static int vwlan_netdev_event(struct notifier_block *this,
			    unsigned long event, void *ptr)
{
	struct net_device *dev = ptr;

	switch (event) {
	case NETDEV_UP:
		bdbg(LV_0324_CHK_BASE_DEV, "NETDEV_UP: dev[%s]\n", dev->name);
		if (strcmp(dev->name, REAL_DEV) == 0) {
			atomic_set(&base_dev_running, 1);
			bdbg(LV_0324_CHK_BASE_DEV, 
				"NETDEV_UP: base_dev_running[%d]\n", 
			atomic_read(&base_dev_running));			
		}
		if (strcmp(dev->name, "vwlan") == 0) {
			atomic_set(&vrit_dev_running, 1);
			bdbg(LV_0324_CHK_BASE_DEV, 
				"NETDEV_UP: vrit_dev_running[%d]\n", 
				atomic_read(&vrit_dev_running));			
		}
		break;
	case NETDEV_DOWN:
		bdbg(LV_0324_CHK_BASE_DEV, "NETDEV_DOWN: dev[%s]\n", dev->name);
		if (strcmp(dev->name, REAL_DEV) == 0) {
			atomic_set(&base_dev_running, 0);
			bdbg(LV_0324_CHK_BASE_DEV, 
				"NETDEV_DOWN: base_dev_running[%d]\n", 
				atomic_read(&base_dev_running));
		}
		if (strcmp(dev->name, "vwlan") == 0) {
			bdbg(LV_0324_CHK_BASE_DEV, 
				"NETDEV_DOWN: vrit_dev_running[%d]\n", 
				atomic_read(&vrit_dev_running));			
			atomic_set(&vrit_dev_running, 0);
		}
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int vwlan_inetaddr_event(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
	struct net_device *dev = ifa->ifa_dev->dev;
	int i;

	switch (event) {
	case NETDEV_UP:
		bdbg(LV_0324_CHK_BASE_DEV, "NETDEV_UP: dev[%s]\n", dev->name);
		if (dev == dev_get_by_name(&init_net, REAL_DEV)) {
			base_dev = dev;
			bdbg(LV_0324_CHK_BASE_DEV, "\n");
			bdbg(LV_DEBUG, "dev:%s, ip:0x%x\n", dev->name, ifa->ifa_local);
			if (atomic_read(&vrit_dev_running) && ifa->ifa_local == 0x0AFDFD02) {
				if (socket_created == 0) {
					vwlan_socket_create();
				}
			}
		}
		else {
			bdbg(LV_0324_CHK_BASE_DEV, "dev:%s, ip:0x%x\n", dev->name, ifa->ifa_local);
			if (strcmp(dev->name, "vwlan") == 0) {
				bdbg(LV_DEBUG, "dev:%s, ip:0x%x\n", dev->name, ifa->ifa_local);
				if (ifa->ifa_local == 0x0AFDFD02) {
					bdbg(LV_DEBUG, "dev:%s, ip:0x%x\n", dev->name, ifa->ifa_local);
					if (socket_created == 0) {
						bdbg(LV_DEBUG, "dev:%s, ip:0x%x\n", dev->name, ifa->ifa_local);
						vwlan_socket_create();
					}
				}
			}

			/* save ip address */		
			for (i = 0; i < dev_num; i++) {
				if (dev == vwlan_dev_table[i].dev) {
					bdbg(LV_0324_CHK_BASE_DEV, 
						"NETDEV_UP, dev[%s], ip[0x%x]\n", 
						dev->name, ifa->ifa_local);

					vwlan_dev_table[i].ipaddr = ifa->ifa_local;
					break;
				}
			}
		}
		break;
	case NETDEV_DOWN:
		bdbg(LV_0324_CHK_BASE_DEV, "NETDEV_DOWN: dev[%s]\n", dev->name);
		/* clean saved ip address */
		for (i = 0; i < dev_num; i++) {
			if (dev == vwlan_dev_table[i].dev) {
				bdbg(LV_0324_CHK_BASE_DEV, 
					"NETDEV_DOWN, dev[%s]\n", 
					dev->name);

				vwlan_dev_table[i].ipaddr = 0;
				break;
			}
		}
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block vwlan_netdev_notifier = {
	.notifier_call = vwlan_netdev_event,
};

static struct notifier_block vwlan_inetaddr_notifier = {
	.notifier_call = vwlan_inetaddr_event,
};

//-- netdev_ops ------------------------------------------------------------------
static int vwlan_xmit(struct sk_buff *skb, struct net_device *dev);

static int vwlan_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;

	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}

/* fake multicast ability */
static void set_multicast_list(struct net_device *dev)
{
	bdbg(LV_DEBUG, "do nothing\n");
}

static void poll_func(unsigned long data) {
	bdbg(LV_DEBUG, "%timeout!\n");
	kill_pid(find_get_pid(data), SIGTERM, 1);
	mutex_unlock(&pm_mutex);
}

static int vwlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int size = -1, ret;
	char type = CMD_IOCTL;
	struct iw_priv_args priv[48];
	struct iwreq *wrq = (struct iwreq *) ifr;
	struct iwreq twrq;
	struct timer_list tmr_poll;
	int send_len = VWLAN_PLEN;
	pid_t user_pid;
	vwlan_packet_t *vwlan_pkt;

	bdbg(LV_TRACE, "\n");

	if (is_rpc_ready())
		return 0;

	/* get user pid */
	user_pid = current->pid;
	bdbg(LV_TRACE, "user pid %d\n", user_pid);

	if(cmd == SIOCGIMCAST_ADD || cmd == SIOCGIMCAST_DEL){
		return 0;
	}

	if (!mutex_trylock(&pm_mutex)){
		bdbg(LV_DEBUG, "mutex_trylock\n");
		return -1;
	}

	bdbg(LV_TRACE, "\n");

	vwlan_pkt = (vwlan_packet_t *)(buffer);

	/* clear buf */
	memset(vwlan_pkt, 0, MAXLENGTH);

	/* cp type to buf */
	vwlan_pkt->cmd_type = type;

	/* cp iwreq struct to buf */
	memcpy(&vwlan_pkt->wrq, wrq, sizeof(struct iwreq));

	/* cp cmd to buf */
	vwlan_pkt->cmd = cmd;
	bdbg(LV_TRACE, "cmd: %x\n", cmd);

	switch (cmd){
		case SIOCGIWNAME:
		case SIOCGMIIPHY:
		//case SIOCGIWPRIV:
			bdbg(LV_TRACE, "cmd: %x, no copy\n", cmd);
			break;
		default:
			bdbg(LV_TRACE, "cmd: %x, copy_from_user\n", cmd);
			copy_from_user(vwlan_pkt->data, 
				(void *)wrq->u.data.pointer, wrq->u.data.length);
			send_len += wrq->u.data.length;
			break;
	}

	/* send packet with format: iwreq / cmd / iwreq data content */
	size = ksocket_send(sock_send, &addr_send, buffer, send_len);
	bdbg(LV_DEBUG, "cmd: %x, send size[%d]\n", cmd, size);

recieve_pkt:
	/* start timer */
	init_timer(&tmr_poll);
	//tmr_poll.data = 0;
	tmr_poll.data = user_pid;
	tmr_poll.function = poll_func;
	mod_timer(&tmr_poll, jiffies + CMD_TIMEOUT);
	/* clear buf */
	memset(buffer, 0, MAXLENGTH);

	/* receive packet with format: ret / iwreq / iwreq data content */
	size = -1;
	size = ksocket_receive(sock, &addr, buffer, MAXLENGTH);
	bdbg(LV_DEBUG, "cmd: %x, rcv size[%d]\n", cmd, size);

	/* del timer if receive */
	del_timer(&tmr_poll);

	if(size >= 0){
		bdbg(LV_TRACE, "recieved\n");
		if(cmd == SIOCSAPPPID){
			bdbg(LV_DEBUG, "SIOCSAPPPID\n");
			signal_socket_end();
			signal_socket_start();
		}

		/* check if ioctl failed */
		if(vwlan_pkt->ret==0) {
			bdbg(LV_TRACE, "ioctl failed\n");
			ret = -1;
			goto UN_LOCK;
		}

		/* temporarily copy ioctl result */
		memcpy(&twrq, &vwlan_pkt->wrq, sizeof(struct iwreq));

		/* cp iwreq data content from recieved packet */
		switch (cmd){
			case SIOCGIWPRIV:
				bdbg(LV_DEBUG, "cmd: %x\n", cmd);
				copy_to_user((void *)wrq->u.data.pointer, 
					vwlan_pkt->data, sizeof(priv));
				break;
			case SIOCGIWNAME:
				bdbg(LV_DEBUG, "cmd: %x\n", cmd);
				copy_to_user((void *)wrq->u.name, 
					twrq.u.name, sizeof(twrq.u.name));
				ret = 0;
				goto UN_LOCK;
			default:
				copy_to_user((void *)wrq->u.data.pointer, 
					vwlan_pkt->data, twrq.u.data.length);
				break;
		}
		/* cp iwreq data length & flags from recieved packet */
		__put_user(twrq.u.data.length, &wrq->u.data.length);
		__put_user(twrq.u.data.flags, &wrq->u.data.flags);
		ret = 0;
		goto UN_LOCK;
	}
	else{
		bdbg_err("error getting datagram, sock_recvmsg error = %d\n", 
			size);

		goto recieve_pkt;
	}

UN_LOCK:
	mutex_unlock(&pm_mutex);
	return ret;

}

void _vwlan_rpc_cfg(vwlan_cfg80211_t *cfg)
{
	char buf[sizeof(vwlan_cfg_packet_t)];
	
	vwlan_cfg_packet_t *vp = (vwlan_cfg_packet_t *)buf;
	int size = -1;

	memset(buf, '\0', sizeof(buf)); 

	vp->cmd_type = CMD_RPCCFG;
	memcpy(&vp->cfg, cfg, sizeof(vwlan_cfg80211_t));

	bdbg(LV_TRACE, "send: cfg.cmd[0x%x] cfg.dev_name[%s]\n", 
		vp->cfg.cmd, vp->cfg.dev_name);
	ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
	size = ksocket_receive(sock, &addr, buf, sizeof(buf));

	if(size >= 0){
		bdbg(LV_0314_RX_DEV, "recieved\n");
	}
	memcpy(cfg, &vp->cfg, sizeof(cfg));

	bdbg(LV_TRACE, "recieved: cfg.cmd=0x%x cfg.dev_name=%s cfg.ret_val=%d\n", 
		vp->cfg.cmd, vp->cfg.dev_name, vp->cfg.ret_val);

	return;
}

static struct net_device_stats *_vwlan_get_stats(struct net_device *dev)
{
	char buf[sizeof(vwlan_ifstats_packet_t)];
	vwlan_ifstats_packet_t *vp = (vwlan_ifstats_packet_t *)buf;
	int size = -1;

	bdbg(LV_0314_RX_DEV, "\n");

	memset(buf, '\0', sizeof(buf)); 

	vp->cmd_type = CMD_GETSTATE;
	memcpy(vp->ifname, dev->name, IFNAMSIZ);

	ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
	size = ksocket_receive(sock, &addr, buf, sizeof(buf));

	if(size >= 0){
		bdbg(LV_0314_RX_DEV, "recieved\n");
		memcpy(&dev->stats, &vp->stats, sizeof(struct net_device_stats));
	}

	return &dev->stats;
}

struct net_device_stats *vwlan_get_stats(struct net_device *dev)
{
	struct vwlan_priv *priv = netdev_priv(dev);

	if (is_rpc_ready()) {
		return &priv->status;
	}

	return _vwlan_get_stats(dev);
}

static void vwlan_open_send(char *devname)
{
	char buf[sizeof(vwlan_if_packet_t)];
	vwlan_if_packet_t *vp;
	vp = (vwlan_if_packet_t *)buf;

	memset(buf, '\0', sizeof(buf));

	vp->cmd_type = CMD_OPEN;
	memcpy(vp->ifname, devname, IFNAMSIZ);

	ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
}

static int vwlan_open(struct net_device *dev)
{
	if (is_rpc_ready())
		return 0;

	vwlan_open_send(dev->name);
	return 0;
}

static void vwlan_close_send(char *devname)
{
	char buf[sizeof(vwlan_if_packet_t)];
	vwlan_if_packet_t *vp;
	vp = (vwlan_if_packet_t *)buf;

	memset(buf, '\0', sizeof(buf));

	vp->cmd_type = CMD_CLOSE;
	memcpy(vp->ifname, devname, IFNAMSIZ);

	ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
}

static int vwlan_close(struct net_device *dev)
{
	if (is_rpc_ready())
		return 0;

	vwlan_close_send(dev->name);
	return 0;
}

static const struct net_device_ops vwlan_netdev_ops = {
	.ndo_start_xmit		= vwlan_xmit,
	.ndo_validate_addr	= eth_validate_addr,
	//.ndo_set_multicast_list = set_multicast_list,
	.ndo_set_mac_address	= vwlan_set_address,

	/* for control wifi */
	.ndo_open		= vwlan_open,
	.ndo_stop		= vwlan_close,
	.ndo_get_stats		= vwlan_get_stats,
	.ndo_do_ioctl		= vwlan_ioctl,
	//.ndo_tx_timeout		= vwlan_tx_timeout,
	//.ndo_change_mtu		= vwlan_set_mtu,
};

static void vwlan_setup(struct net_device *dev)
{
	ether_setup(dev);

	/* Initialize the device structure. */
	dev->netdev_ops = &vwlan_netdev_ops;
	dev->destructor = free_netdev;

	/* Fill in device structure with ethernet-generic values. */
	dev->tx_queue_len = 0;
	random_ether_addr(dev->dev_addr);
}

static int vwlan_xmit(struct sk_buff *skb, struct net_device *dev)
{
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	if (base_dev) {
		skb->dev = dev = base_dev;
	}
	else {
		dev_kfree_skb(skb);
		return 0;
	}

	return base_dev->netdev_ops->ndo_start_xmit(skb, dev);
}

static int vwlan_validate(struct nlattr *tb[], struct nlattr *data[])
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	return 0;
}

static struct rtnl_link_ops vwlan_link_ops  __read_mostly = {
	.kind		= MODULE_NAME,
	.setup		= vwlan_setup,
	.validate	= vwlan_validate,
};

static int vwlan_init_one(int idx)
{
	struct net_device *vwlan_dev;
	int err;

	vwlan_dev = alloc_netdev(sizeof(struct vwlan_priv), 
		vwlan_dev_table[idx].ifname, vwlan_setup);

	if (!vwlan_dev)
		return -ENOMEM;

	err = dev_alloc_name(vwlan_dev, vwlan_dev->name);
	if (err < 0)
		goto err;

	vwlan_dev->rtnl_link_ops = &vwlan_link_ops;
	memcpy(vwlan_dev->dev_addr, vwlan_dev_table[idx].dev_addr,  ETH_ALEN);

	err = register_netdevice(vwlan_dev);
	if (err < 0)
		goto err;

	/* reg rx_handler to dispatch packet from nic to vwlan */
	vwlan_rx_handler_reg(vwlan_dev);

	vwlan_dev_table[idx].dev = vwlan_dev;
	printk("vwlan: reg %s, ok.\n", vwlan_dev_table[idx].dev->name);
	return 0;

err:
	printk("vwlan: reg %s fail.\n", vwlan_dev_table[idx].dev->name);
	free_netdev(vwlan_dev);
	return err;
}

#define WITH_CFG80211
static int __init vwlan_init(void)
{
	int i, err = 0;

	rtnl_lock();
#ifndef WITH_CFG80211
	dev_num = ((sizeof(vwlan_dev_table))/(sizeof(vwlan_dev_config_t)));
#else
	/* init wlan0/wlan1 by vwlan_cfg80211_init */
	dev_num = 1;
#endif

	if (vwlan_running == 0) {
		vwlan_running = 1;
		err = __rtnl_link_register(&vwlan_link_ops);

		for (i = 0; i < dev_num && !err; i++) {
			bdbg(LV_DEBUG, "%d, %s\n", i, vwlan_dev_table[i].ifname);
			err = vwlan_init_one(i);
		}
		if (err < 0) {
			vwlan_rx_handler_unreg();
			__rtnl_link_unregister(&vwlan_link_ops);
		}
		vwlan_create_devices_proc();
	}
	rtnl_unlock();

	/* reg rx_dev change */
	register_netdevice_notifier(&vwlan_netdev_notifier);

	/* reg for ip change  */
	register_inetaddr_notifier(&vwlan_inetaddr_notifier);

#ifdef WITH_CFG80211
	vwlan_cfg80211_init();
#endif

	return err;
}

static void __exit vwlan_cleanup(void)
{
	rtnl_lock();

	vwlan_rx_handler_unreg();
	vwlan_remove_devices_proc();

	sock_release(sock);
	sock_release(sock_send);
	socket_created = 0;
  
	vwlan_running = 0;
	rtnl_unlock();
	
	unregister_inetaddr_notifier(&vwlan_inetaddr_notifier);
	unregister_netdevice_notifier(&vwlan_netdev_notifier);

	rtnl_link_unregister(&vwlan_link_ops);
}

module_init(vwlan_init);
module_exit(vwlan_cleanup);
MODULE_LICENSE("GPL");
//MODULE_ALIAS_RTNL_LINK(MODULE_NAME);

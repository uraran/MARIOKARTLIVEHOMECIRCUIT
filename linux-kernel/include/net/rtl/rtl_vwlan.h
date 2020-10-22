#ifndef	_RTL_VWLAN_H_
#define	_RTL_VWLAN_H_


#define USE_RPC_IPADDR
//#define VWLAN_LEFT_SIDE

#if !defined(VWLAN_LEFT_SIDE)
//#define VWLAN_RIGHT_BYPASS_BRSC
#endif

/* for fragment event indicate packet */
#define IND_STR_LEN 3000
#define FRAG_SZ 1400

#define SSID_MAX_LEN 33
#define IES_MAX_LEN (256*4)
#define IE_MAX_LEN (256)
#define MAX_REQUEST_CHANNELS 20

#define REPORT_SCAN_IES_LEN 512

#define MAX_ASSOC_REQ_LEN 	512
#define MAX_ASSOC_RSP_LEN 	512


#if !defined(VWLAN_FOR_USERS)
struct rtk_cfg80211_scan{
    //struct cfg80211_scan_request *requestPtr;  // keep pointer for call cfg80211xxx        
    u8 ssid[SSID_MAX_LEN];
    u8 ssid_len;       
    
    u8 ie[IES_MAX_LEN];
    u8 ie_len;

    u32 n_channels;
    u32 cfg_channels[50];
};
struct cfg80211_mgmt_tx{
       //struct cfg80211_mgmt_tx_params params,
        u8 mgmt_tx_channel;
        const u8 buf[1024];
        int buf_len;
        //u64 cookie;
};
struct cfg80211_remain_on_channel{
    struct ieee80211_channel remain_on_ch_channel;    
    u8 re_channel;// for easy use
    unsigned int duration;
    long long unsigned int cookie;
};
struct cfg80211_remain_on_chnl_expired{         
     struct ieee80211_channel remain_on_ch_channel;           
     long long unsigned int remain_on_ch_cookie;
};

struct cfg80211_rx_mgmt{            
     int freq;
     int sig_dbm;            
     u8  rx_buf[1024];
     size_t rx_len;
     u32 flags;
};
struct cfg80211_connect_result{         
    //get bss
    unsigned char   chnl;
    unsigned char   bssid1[ETH_ALEN];
    unsigned char   ssid[32];
    unsigned short  ssidlen;

    //bss info / ss result info
    //struct ieee80211_channel channel;     // !!wrong way don't use!!
    u8 channel_ch;
    
    u8  mac[ETH_ALEN];
    unsigned long   timestamp;
    unsigned short  capability;
    unsigned short  beacon_prd;
    unsigned char   ie[REPORT_SCAN_IES_LEN];
    unsigned char   ie_len;
    unsigned char   rssi;

    //connect result
    unsigned char bssid2[ETH_ALEN];
    unsigned char assoc_req[MAX_ASSOC_REQ_LEN];
    unsigned char assoc_req_len;
    unsigned char assoc_rsp[MAX_ASSOC_RSP_LEN];
    unsigned char assoc_rsp_len;
};

#endif


//------------------------------------------------------------------------------
enum cfg80211_cmd {
	CMD_CFG80211_ADD_IFACE,//0
	CMD_CFG80211_DEL_IFACE,
	CMD_CFG80211_CHANGE_IFACE,
	CMD_CFG80211_ADD_KEY,
	CMD_CFG80211_DEL_KEY,
	CMD_CFG80211_GET_KEY,//5
	CMD_CFG80211_SET_DEFAULT_KEY,
	CMD_CFG80211_SET_DEFAULT_MGMT_KEY,
	CMD_CFG80211_AUTH,
	CMD_CFG80211_ASSOC,
	CMD_CFG80211_DEAUTH,//10
	CMD_CFG80211_DISASSOC,
	CMD_CFG80211_ADD_STATION,
	CMD_CFG80211_DEL_STATION,
	CMD_CFG80211_CHANGE_STATION,
	CMD_CFG80211_GET_STATION,//15
	CMD_CFG80211_DUMP_STATION,
	CMD_CFG80211_SET_WIPHY_PARAMS,
	CMD_CFG80211_SET_TX_POWER,
	CMD_CFG80211_GET_TX_POWER,
	CMD_CFG80211_SUSPEND,//20
	CMD_CFG80211_RESUME,
	CMD_CFG80211_SCAN,//22
	CMD_CFG80211_JOIN_IBSS,
	CMD_CFG80211_LEAVE_IBSS,
	CMD_CFG80211_SET_WDS_PEER,//25
	CMD_CFG80211_RFKILL_POLL,
	CMD_CFG80211_SET_POWER_MGMT,
	CMD_CFG80211_SET_BITRATE_MASK,
	CMD_CFG80211_CONNECT,//29
	CMD_CFG80211_CHANGE_BSS,//30
	CMD_CFG80211_DISCONNECT,
	CMD_CFG80211_REMAIN_ON_CHANNEL,
	CMD_CFG80211_CANCEL_REMAIN_ON_CHANNEL,
	CMD_CFG80211_CHANNEL_SWITCH,
	CMD_CFG80211_MGMT_TX,//35
	CMD_CFG80211_MGMT_FRAME_REGISTER,
	CMD_CFG80211_START_AP,//37
	CMD_CFG80211_CHANGE_BEACON,
	CMD_CFG80211_STOP_AP,
	CMD_CFG80211_NULL,
};

/*-----------------------2014-0407--------todo--------------------*/
#define stringiz(arg) #arg

/*left side cmd to right side*/
static const char cfg80211_cmd_str[CMD_CFG80211_NULL][64] = {
	stringiz(CMD_CFG80211_ADD_IFACE),
	stringiz(CMD_CFG80211_DEL_IFACE),
	stringiz(CMD_CFG80211_CHANGE_IFACE),
	stringiz(CMD_CFG80211_ADD_KEY),
	stringiz(CMD_CFG80211_DEL_KEY),
	stringiz(CMD_CFG80211_GET_KEY),
	stringiz(CMD_CFG80211_SET_DEFAULT_KEY),
	stringiz(CMD_CFG80211_SET_DEFAULT_MGMT_KEY),
	stringiz(CMD_CFG80211_AUTH),
	stringiz(CMD_CFG80211_ASSOC),
	stringiz(CMD_CFG80211_DEAUTH),
	stringiz(CMD_CFG80211_DISASSOC),
	stringiz(CMD_CFG80211_ADD_STATION),
	stringiz(CMD_CFG80211_DEL_STATION),
	stringiz(CMD_CFG80211_CHANGE_STATION),
	stringiz(CMD_CFG80211_GET_STATION),
	stringiz(CMD_CFG80211_DUMP_STATION),
	stringiz(CMD_CFG80211_SET_WIPHY_PARAMS),
	stringiz(CMD_CFG80211_SET_TX_POWER),
	stringiz(CMD_CFG80211_GET_TX_POWER),
	stringiz(CMD_CFG80211_SUSPEND),
	stringiz(CMD_CFG80211_RESUME),
	stringiz(CMD_CFG80211_SCAN),
	stringiz(CMD_CFG80211_JOIN_IBSS), // 22
	stringiz(CMD_CFG80211_LEAVE_IBSS),
	stringiz(CMD_CFG80211_SET_WDS_PEER),
	stringiz(CMD_CFG80211_RFKILL_POLL),
	stringiz(CMD_CFG80211_SET_POWER_MGMT),
	stringiz(CMD_CFG80211_SET_BITRATE_MASK),
	stringiz(CMD_CFG80211_CONNECT),
	stringiz(CMD_CFG80211_CHANGE_BSS),
	stringiz(CMD_CFG80211_DISCONNECT),
	stringiz(CMD_CFG80211_REMAIN_ON_CHANNEL),
	stringiz(CMD_CFG80211_CANCEL_REMAIN_ON_CHANNEL),
	stringiz(CMD_CFG80211_CHANNEL_SWITCH),
	stringiz(CMD_CFG80211_MGMT_TX),
	stringiz(CMD_CFG80211_MGMT_FRAME_REGISTER),
	stringiz(CMD_CFG80211_START_AP),
	stringiz(CMD_CFG80211_CHANGE_BEACON),
	stringiz(CMD_CFG80211_STOP_AP),

};
/*-----------------------2014-0407----------------------------*/


/*-----------------------2014-0408----------------------------*/
#if 0
/*right side event indicate to left side*/

enum RTK_CFG80211_EVENT {
	CFG80211_CONNECT_RESULT = 0,
	CFG80211_ROAMED = 1,
	CFG80211_DISCONNECTED = 2,
	CFG80211_IBSS_JOINED = 3,
	CFG80211_NEW_STA = 4,
	CFG80211_SCAN_DONE = 5, 
	CFG80211_DEL_STA = 6, 
	CFG80211_SS_RESULT = 7, 
	CFG80211_NULL = 8, 
};

static const char cfg80211_event_str[CFG80211_NULL][64] = {
	stringiz(CFG80211_CONNECT_RESULT),
	stringiz(CFG80211_ROAMED),
	stringiz(CFG80211_DISCONNECTED),
	stringiz(CFG80211_IBSS_JOINED),
	stringiz(CFG80211_NEW_STA),
	stringiz(CFG80211_SCAN_DONE),
	stringiz(CFG80211_DEL_STA),
	stringiz(CFG80211_SS_RESULT),
};

#endif
/*-----------------------2014-0408----------------------------*/


//Linux to Virtual
typedef struct vwlan_cfg80211_event_ind_s
{
	char dev_name[IFNAMSIZ]; /* keep this dev_name[] at first */
	int cmd;
	int ret_val;

#if !defined(VWLAN_FOR_USERS)
	//must remove
	union {
		//char str[IND_STR_LEN]; //for test, remove

		struct cfg80211_connect_result connect_result;

		struct cfg80211_roamed{			
			;///No Data
		}roamed;

		struct cfg80211_disconnected{			
			;//No Data
		}disconnected;

		struct cfg80211_ibss_joined{			
			unsigned char bssid[ETH_ALEN];
		}ibss_joined;

		struct cfg80211_new_sta{			
			unsigned char mac[ETH_ALEN];
			struct station_info sinfo;
		}new_sta;

		struct cfg80211_scan_done{			
			;///No Data
		}scan_done;

		struct cfg80211_del_sta{			
			unsigned char mac[ETH_ALEN];
		}del_sta;

		struct cfg80211_remain_on_chnl_expired remain_on_chnl_expired;

		struct cfg80211_rx_mgmt rx_mgmt;		
	};
#endif /* #if !defined(VWLAN_FOR_USERS)  */
}__attribute__((packed)) vwlan_cfg80211_event_ind_t;

//Virtual to Linux
typedef struct vwlan_cfg80211_s
{
	char dev_name[IFNAMSIZ];
	int cmd;
	int ret_val;

#if !defined(VWLAN_FOR_USERS)
 
	union {
		struct cfg80211_add_iface{   
			enum nl80211_iftype type;
			char name[IFNAMSIZ];
		}add_iface;

		struct cfg80211_del_iface{
			;
		}del_iface;

		struct cfg80211_change_iface{
			enum nl80211_iftype type;
		}change_iface;

		struct cfg80211_add_key{
			u8 key_index;
			bool pairwise;
			u8 mac_addr[ETH_ALEN];
			//key_param
			u8 key[256];
			int key_len;
			u32 cipher;
		}add_key;

		struct cfg80211_del_key{
			u8 key_index;
			bool pairwise;
			u8 mac_addr[ETH_ALEN];
		}del_key;

		struct cfg80211_get_key{
			u8 key_index;
			bool pairwise;
			u8 mac_addr[ETH_ALEN];
			//key_param
			u8 key[256];
			int key_len;
			u32 cipher;
			//use in virtual
			//void *cookie;
			//void (*callback) (void *cookie, struct key_params *);
		}get_key;

		struct cfg80211_set_default_key{
			u8 key_index;
			bool unicast;
			bool multicast;
		}set_default_key;

		struct cfg80211_set_default_mgmt_key{
			u8 key_idx;
		}set_default_mgmt_key;

		struct cfg80211_auth{
			struct cfg80211_auth_request req;
		}auth;

		struct cfg80211_assoc{
			struct cfg80211_assoc_request req;
		}assoc;

		struct cfg80211_deauth{
			struct cfg80211_deauth_request req;
			void *cookie;
		}deauth;

		struct cfg80211_disassoc{
			struct cfg80211_disassoc_request req;
			void *cookie;
		}disassoc;

		struct cfg80211_add_station{
			u8 mac[ETH_ALEN];
			//struct station_parameters params;
		}add_station;

		struct cfg80211_del_station{
			u8 mac[ETH_ALEN];
		}del_station;

		struct cfg80211_change_station{
			u8 mac[ETH_ALEN];
			//struct station_parameters params;
			u32 sta_flags_set;
		}change_station;

		struct cfg80211_get_station{
			u8 mac[ETH_ALEN];
			struct station_info sinfo;
		}get_station;

		struct cfg80211_dump_station{
			int idx;
			u8 mac[ETH_ALEN];
			struct station_info sinfo;
		}dump_station;

		struct cfg80211_set_wiphy_params{
			u32 changed;
			u8 retry_short;
			u8 retry_long;
			u32 rts_threshold;
		}set_wiphy_params;

		struct cfg80211_set_tx_power{
			enum nl80211_tx_power_setting type;
			int mbm;
		}set_tx_power;

		struct cfg80211_get_tx_power{
			int dbm;
		}get_tx_power;

		struct cfg80211_suspend{
			;//struct cfg80211_wowlan *wow;
		}suspend;

		struct cfg80211_resume{
		;
		}resume;

                struct rtk_cfg80211_scan scan;

		struct cfg80211_mgmt_tx mgmt_tx;

		struct cfg80211_join_ibss{
			//struct cfg80211_ibss_params *ibss_param;
			u8 ssid[33];
			u8 ssid_len;
			bool privacy;
		}join_ibss;

		struct cfg80211_leave_ibss{
			;
		}leave_ibss;

		struct cfg80211_set_wds_peer{
			u8 addr[ETH_ALEN];
		}set_wds_peer;

		struct cfg80211_set_power_mgmt{
			bool enabled;
			int timeout;
		}set_power_mgmt;

		struct cfg80211_set_bitrate_mask{
			const u8 addr[ETH_ALEN];
			const struct cfg80211_bitrate_mask mask;
		}set_bitrate_mask;

		struct cfg80211_connect{
			//struct cfg80211_connect_params *sme;
			u8 bssid[6];
			u8 no_bssid; 
			u8 ssid[33];
			u8 ssid_len;
			u8 ie[512];
			u8 ie_len;
			struct cfg80211_crypto_settings crypto;
			const u8 key[256];
			u8 key_len; 
			u8 key_idx;
		}connect;

		struct cfg80211_change_bss{
			//struct bss_parameters *params;
			int use_short_preamble;
			u8 basic_rates[256];
			u8 basic_rates_len;
		}change_bss;

		struct cfg80211_disconnect{
			u16 reason_code;
		}disconnect;

		struct cfg80211_remain_on_channel remain_on_channel;

		struct cfg80211_cancel_remain_on_channel{
			long long unsigned int  cookie;
		}cancel_remain_on_channel;

		struct cfg80211_channel_switch{
			//need to fix here
			;//struct cfg80211_csa_settings params;
		}channel_switch;




		struct cfg80211_mgmt_frame_register{
			//int ret_val;
			u16 frame_type;
			bool reg;
		}mgmt_frame_register;

		struct cfg80211_start_ap{
			//struct cfg80211_ap_settings *info;
			//struct cfg80211_chan_def chandef;
			struct ieee80211_channel chan;
			enum nl80211_chan_width width;
			//struct cfg80211_beacon_data beacon;
			const u8 beacon_ies[256];
			const u8 proberesp_ies[256];
			const u8 assocresp_ies[256];
			size_t beacon_ies_len;
			size_t proberesp_ies_len;
			size_t assocresp_ies_len;
			const u8 ssid[32];
			size_t ssid_len;
			enum nl80211_hidden_ssid hidden_ssid;
			struct cfg80211_crypto_settings crypto;
			enum nl80211_auth_type auth_type;
		}start_ap;

		struct cfg80211_change_beacon{
			//struct cfg80211_beacon_data *beacon;
			const u8 beacon_ies[256];
			const u8 proberesp_ies[256];
			const u8 assocresp_ies[256];
			size_t beacon_ies_len;
			size_t proberesp_ies_len;
			size_t assocresp_ies_len;
		}change_beacon;

		struct cfg80211_stop_ap{
			;
		}stop_ap;
	};
#endif /* #if !defined(VWLAN_FOR_USERS)  */
} __attribute__((packed)) vwlan_cfg80211_t;

void _vwlan_rpc_cfg(vwlan_cfg80211_t *cfg);

//------------------------------------------------------------------------------
#define DEFAULT_PORT 	32325
#define SIG_PORT 	32313
#define SERV_PORT 		(DEFAULT_PORT)

#define SIOCGIWPRIV 	0x8B0D
#define SIOCSAPPPID     0x8b3e
#define SIOCGIWNAME     0x8B01
#define SIOCGMIIPHY		0x8947
//#ifdef SUPPORT_TX_MCAST2UNI
#define SIOCGIMCAST_ADD			0x8B80
#define SIOCGIMCAST_DEL			0x8B81
//#endif
#define SIOCDEVSTATS	0x8B5E
#define SIOCRPCCFG 		0x8B5F
#define SIOCRPCCFGEVT	0x8B60 // just for test indicate, remove it after finish development
#define RTL8192CD_IOCTL_FROM_ANDROID	    0x8BDA	


#define CMD_TIMEOUT 	5*HZ
#define CMD_IOCTL		0
#define CMD_OPEN		1
#define CMD_CLOSE		2
#define CMD_SETHWADD	3
#define CMD_GETSTATE	4
#define CMD_FORCESTOP	5
#define CMD_PROCREAD	6
#define CMD_PROCWRITE	7
#define CMD_RPCCFG		8
#define CMD_INDCATE_EVT_TEST 9  // just for test indicate, remove it after finish development


#define	ETHER_ADDRLEN	6
#define ETHER_HDRLEN 	14

#if defined(USE_RPC_IPADDR)
#define INADDR_SLAVE  ((unsigned long int)0x0AFDFD01) //10.253.253.1
#define INADDR_MASTER ((unsigned long int)0x0AFDFD02) //10.253.253.2
#else
#define INADDR_SLAVE  ((unsigned long int)0xC0A80101) //192.168.1.1
#define INADDR_MASTER ((unsigned long int)0xC0A801FC) //192.168.1.252
#endif

#define MAXLENGTH 	(64000)	
#define DATALENGTH	(63900)

#define LENGTH		56
#define IFNAMSIZ	16

#define VWLAN_PLEN	(sizeof(vwlan_packet_t) - DATALENGTH)

struct user_net_device_stats {
	unsigned long	rx_packets;
	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	unsigned long	multicast;
	unsigned long	collisions;
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;
	unsigned long	rx_crc_errors;
	unsigned long	rx_frame_errors;
	unsigned long	rx_fifo_errors;
	unsigned long	rx_missed_errors;
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

typedef struct vwlan_packet
{
	char cmd_type;
	struct iwreq wrq;
	int cmd;
	char ret;
	char data[DATALENGTH];
} vwlan_packet_t;

typedef struct vwlan_sig_packet
{
	char ret;
	char data[sizeof(pid_t)];
} vwlan_sig_packet_t;

typedef struct vwlan_if_packet
{
	char cmd_type;
	char ifname[IFNAMSIZ];
	char hwaddr[ETHER_ADDRLEN*3];
} vwlan_if_packet_t;

typedef struct vwlan_ifstats_packet
{
	char cmd_type;
	char ifname[IFNAMSIZ];
#if defined(VWLAN_FOR_USERS)
	struct user_net_device_stats stats;
#else
	struct net_device_stats stats;
#endif
} vwlan_ifstats_packet_t;

typedef struct vwlan_proc_packet
{
	char cmd_type;
	int len;
	int offset;
	char name[256];
	char data[8192];
} vwlan_proc_packet_t;

typedef struct vwlan_cfg_packet
{
	char cmd_type;
	vwlan_cfg80211_t cfg;
} vwlan_cfg_packet_t;

struct kthread_t
{
	struct task_struct *thread;
	struct socket *sock;
	struct sockaddr_in addr;
	int running;
};

//------------------------------------------------------------------------------
enum iface_idx {
	IFACE_VWLAN=1,
	IFACE_WLAN0_0,
	IFACE_WLAN0_1,
	IFACE_WLAN0_2,
	IFACE_WLAN0_3,
	IFACE_WLAN0_4,
	IFACE_WLAN0_5,
	IFACE_WLAN0_6,
	IFACE_WLAN0_7,
	IFACE_WLAN0_8,
	IFACE_WLAN0_9,
	IFACE_WLAN1_0,
	IFACE_WLAN1_1,
	IFACE_WLAN1_2,
	IFACE_WLAN1_3,
	IFACE_WLAN1_4,
	IFACE_WLAN1_5,
	IFACE_WLAN1_6,
	IFACE_WLAN1_7,
	IFACE_WLAN1_8,
	IFACE_WLAN1_9
};

#define ETH_P_RTL_RPC 0xE04C

#define RTL_RPC_IDX	1
#define RTL_RPC_CFG	2

struct rpchdr {
	unsigned char	h_dest[ETH_ALEN];	/* destination eth addr	*/
	unsigned char	h_source[ETH_ALEN];	/* source ether addr	*/
	__be16		h_proto;		/* packet type ID field	*/
	__be16	id;
	__be16	tot_len;
	__be16	offset;
	unsigned char mf;
	unsigned char order;
	unsigned char type; /*idx or cfg */
	union {
		unsigned char idx;
		vwlan_cfg80211_event_ind_t cfg;
	} msg;
} __attribute__((packed));

#if !defined(VWLAN_FOR_USERS)
struct vwlan_priv {
	struct net_device *dev;
	struct vwlan_rtknl		*rtk;
	struct wireless_dev 	wdev;
	int status;
};
#endif

typedef struct vwlan_dev_config_s
{
	unsigned char ifname[IFNAMSIZ];
	char is_root;
	char create_proc;
	struct net_device *dev;
	unsigned char dev_addr[MAX_ADDR_LEN];
	struct proc_dir_entry *proc_dir;
	unsigned int ipaddr;
	char iface_idx;
} vwlan_dev_config_t;

//------------------------------------------------------------------------------
#if !defined(VWLAN_FOR_USERS)
typedef struct proc_priv_s
{
	struct net_device *dev;
	char *cmd;
} proc_priv_t;

typedef struct vwlan_proc_table_s
{
	char *cmd;
	char root_only;
	struct file_operations fops;
	struct proc_dir_entry *entry;
	proc_priv_t priv;
} vwlan_proc_table_t;
#endif

//------------------------------------------------------------------------------
#if !defined(VWLAN_FOR_USERS)
struct arp_packet {
	u8 dest_mac[ETH_ALEN];
	u8 src_mac[ETH_ALEN];
	__be16 type;
	__be16 ar_hrd;
	__be16 ar_pro;
	u8 ar_hln;
	u8 ar_pln;
	__be16 ar_op;
	u8 ar_sha[ETH_ALEN];
	unsigned int ar_sip;
	u8 ar_tha[ETH_ALEN];
	unsigned int ar_tip;
} __packed;
#endif

int vwlan_cfg80211_init(void);

#endif /* _RTL_VWLAN_H_ */

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

//add for plus
#if 1
#define printMac(da)	printk("%02X:%02X:%02X:%02X:%02X:%02X\n",  0xff&*(da), 0xff&*(da+1), 0xff&*(da+2), 0xff&*(da+3), 0xff&*(da+4), 0xff&*(da+5));
#define NDEBUG3 printk
#define	LDEBUG printk
#define NDEBUG2 printk

int is_zero_mac(const unsigned char *mac)
{
	return !(mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]| mac[6]| mac[7]);
}
#define CONFIG_P2P_RTK_SUPPORT
#endif

extern int dev_num;

static struct wireless_dev *realtek_cfg80211_add_iface_vface(struct wiphy *wiphy,
						      const char *name,
						      enum nl80211_iftype type,
						      u32 *flags,
						      struct vif_params *params);
static int realtek_cfg80211_del_iface_vface(struct wiphy *wiphy,
				     struct wireless_dev *wdev);
static int realtek_cfg80211_change_iface_vface(struct wiphy *wiphy,
					struct net_device *ndev,
					enum nl80211_iftype type, u32 *flags,
					struct vif_params *params);
static int realtek_cfg80211_add_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr,
				   struct key_params *params);
static int realtek_cfg80211_del_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr);
static int realtek_cfg80211_get_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr, void *cookie,
				   void (*callback) (void *cookie,
						     struct key_params *));
static int realtek_cfg80211_set_default_key_vface(struct wiphy *wiphy,
					   struct net_device *dev,
					   u8 key_index, bool unicast,
					   bool multicast);
static int realtek_cfg80211_set_default_mgmt_key_vface(struct wiphy *wiphy,
					     struct net_device *dev,
					     u8 key_idx);
static int realtek_cfg80211_add_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac, struct station_parameters *params);
static int realtek_cfg80211_del_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac);
static int realtek_cfg80211_change_station_vface(struct wiphy *wiphy,
				    struct net_device *dev,
				    u8 *mac,
				    struct station_parameters *params);
static int realtek_cfg80211_get_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac, struct station_info *sinfo);
static int realtek_cfg80211_dump_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 int idx, u8 *mac, struct station_info *sinfo);
static int realtek_cfg80211_change_bss_vface(struct wiphy *wiphy,
				struct net_device *dev,
				struct bss_parameters *params);
static int realtek_cfg80211_suspend_vface(struct wiphy *wiphy, struct cfg80211_wowlan *wow);
static int realtek_cfg80211_resume_vface(struct wiphy *wiphy);
static int realtek_cfg80211_scan_vface(struct wiphy *wiphy,
			  struct cfg80211_scan_request *request);
static int realtek_cfg80211_join_ibss_vface(struct wiphy *wiphy, struct net_device *dev,
			       struct cfg80211_ibss_params *ibss_param);
static int realtek_cfg80211_leave_ibss_vface(struct wiphy *wiphy, struct net_device *dev);
static int realtek_cfg80211_set_wiphy_params_vface(struct wiphy *wiphy, u32 changed);
static int realtek_cfg80211_set_tx_power_vface(struct wiphy *wiphy,
				  enum nl80211_tx_power_setting type, int mbm);
static int realtek_cfg80211_get_tx_power_vface(struct wiphy *wiphy,
				  struct wireless_dev *wdev, int *dbm);
static int realtek_cfg80211_set_power_mgmt_vface(struct wiphy *wiphy, struct net_device *dev,
				    bool enabled, int timeout);
static int realtek_cfg80211_set_wds_peer_vface(struct wiphy *wiphy, struct net_device *dev,
				  u8 *addr);
static void realtek_cfg80211_rfkill_poll_vface(struct wiphy *wiphy);
static int realtek_cfg80211_set_bitrate_mask_vface(struct wiphy *wiphy,
				      struct net_device *dev,
				      const u8 *addr,
				      const struct cfg80211_bitrate_mask *mask);
static int realtek_cfg80211_connect_vface(struct wiphy *wiphy, struct net_device *dev,
					  struct cfg80211_connect_params *sme);
static int realtek_cfg80211_disconnect_vface(struct wiphy *wiphy,
						  struct net_device *dev, u16 reason_code);
int realtek_remain_on_channel_vface(struct wiphy *wiphy,
    struct wireless_dev *wdev,
	struct ieee80211_channel *channel,
	unsigned int duration,
	u64 *cookie);
static int realtek_cancel_remain_on_channel_vface(struct wiphy *wiphy,
			struct wireless_dev *wdev,	u64 cookie);
//barry, because there is no cfg80211_csa_settings struct
//static int realtek_cfg80211_channel_switch_vface(struct wiphy *wiphy,
//			struct net_device *dev, struct cfg80211_csa_settings *params);
//barry, because there is no cfg80211_mgmt_tx_params struct
//static int realtek_mgmt_tx_vface(struct wiphy *wiphy, struct wireless_dev *wdev,
//           struct cfg80211_mgmt_tx_params *params,
//           u64 *cookie);

static void realtek_mgmt_frame_register_vface(struct wiphy *wiphy,
				       struct  wireless_dev *wdev,
				       u16 frame_type, bool reg);
static int realtek_start_ap_vface(struct wiphy *wiphy, struct net_device *dev,
			   struct cfg80211_ap_settings *info);
static int realtek_change_beacon_vface(struct wiphy *wiphy, struct net_device *dev,
				struct cfg80211_beacon_data *beacon);
static int realtek_stop_ap_vface(struct wiphy *wiphy, struct net_device *dev);
static int realtek_cfg80211_assoc_vface(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_assoc_request *req);


//this is for NL80211/CFG80211 init
//==================================================

#define VWLAN_RTK_MAX_WIFI_PHY 2
static dev_t vwlan_rtk_wifi_dev[VWLAN_RTK_MAX_WIFI_PHY];
static char *vwlan_rtk_dev_name[VWLAN_RTK_MAX_WIFI_PHY]={"RTKWiFi0","RTKWiFi1"};

extern unsigned int HW_SETTING_OFFSET;
#define HW_WLAN_SETTING_OFFSET	13
#define VIF_NAME_SIZE 10

char vwlan_rtk_fake_addr[2][6]={{0x00,0xe0,0x4c,0x81,0x86,0x87},
						  {0x00,0xe0,0x4c,0x81,0x86,0x86}};

struct vwlan_rtknl {
	struct class *cl;
	struct device *dev;
	struct wiphy *wiphy;
	struct vwlan_rtl8192cd_priv *priv;
	struct net_device *ndev_add;
	unsigned char wiphy_registered;
	unsigned char root_ifname[VIF_NAME_SIZE];
	unsigned char root_mac[ETH_ALEN];
};

typedef struct vwlan_rtl8192cd_priv {
		struct net_device		*dev;
		struct vwlan_rtknl		*rtk;
		struct wireless_dev 	wdev;
} VWLAN_RTL8192CD_PRIV, *VWLAN_PRTL8192CD_PRIV;

static struct device_type vwlan_wiphy_type = {
	.name	= "wlan",
};

static int vwlan_rtk_phy_idx=0;
struct vwlan_rtknl *vwlan_rtk_phy[VWLAN_RTK_MAX_WIFI_PHY];

#define MAX_REMAIN_ON_CHANNEL_DURATION 5000 //ms
#define SCAN_IE_LEN_MAX                2304
#define SSID_SCAN_AMOUNT               1 // for WEXT_CSCAN_AMOUNT 9
#define MAX_NUM_PMKIDS                 32

#define realtek_g_rates     (realtek_rates + 0)
#define realtek_g_rates_size    12

#define realtek_a_rates     (realtek_rates + 4)
#define realtek_a_rates_size    8

#define realtek_g_htcap (IEEE80211_HT_CAP_SUP_WIDTH_20_40 | \
			IEEE80211_HT_CAP_SGI_20		 | \
			IEEE80211_HT_CAP_SGI_40)

#define realtek_a_htcap (IEEE80211_HT_CAP_SUP_WIDTH_20_40 | \
			IEEE80211_HT_CAP_SGI_20		 | \
			IEEE80211_HT_CAP_SGI_40)

static const u32 cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	//CCKM_KRK_CIPHER_SUITE,
	//WLAN_CIPHER_SUITE_SMS4,
};


static const struct ieee80211_txrx_stypes rtw_cfg80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
};

#define CHAN2G(_channel, _freq, _flags) {   \
	.band           = IEEE80211_BAND_2GHZ,  \
	.hw_value       = (_channel),           \
	.center_freq    = (_freq),              \
	.flags          = (_flags),             \
	.max_antenna_gain   = 0,                \
	.max_power      = 30,                   \
}

static struct ieee80211_channel realtek_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	//CHAN2G(14, 2484, 0),
};

#define CHAN5G(_channel, _flags) {                  \
        .band           = IEEE80211_BAND_5GHZ,      \
        .hw_value       = (_channel),               \
        .center_freq    = 5000 + (5 * (_channel)),  \
        .flags          = (_flags),                 \
        .max_antenna_gain   = 0,                    \
        .max_power      = 30,                       \
}

static struct ieee80211_channel realtek_5ghz_a_channels[] = {
        CHAN5G(34, 0), CHAN5G(36, 0),
        CHAN5G(38, 0), CHAN5G(40, 0),
        CHAN5G(42, 0), CHAN5G(44, 0),
        CHAN5G(46, 0), CHAN5G(48, 0),
        CHAN5G(52, 0), CHAN5G(56, 0),
        CHAN5G(60, 0), CHAN5G(64, 0),
        CHAN5G(100, 0), CHAN5G(104, 0),
        CHAN5G(108, 0), CHAN5G(112, 0),
        CHAN5G(116, 0), CHAN5G(120, 0),
        CHAN5G(124, 0), CHAN5G(128, 0),
        CHAN5G(132, 0), CHAN5G(136, 0),
        CHAN5G(140, 0), CHAN5G(149, 0),
        CHAN5G(153, 0), CHAN5G(157, 0),
        CHAN5G(161, 0), CHAN5G(165, 0),
        CHAN5G(184, 0), CHAN5G(188, 0),
        CHAN5G(192, 0), CHAN5G(196, 0),
        CHAN5G(200, 0), CHAN5G(204, 0),
        CHAN5G(208, 0), CHAN5G(212, 0),
        CHAN5G(216, 0),
};

#define RATETAB_ENT(_rate, _rateid, _flags) {   \
	.bitrate    = (_rate),                  \
	.flags      = (_flags),                 \
	.hw_value   = (_rateid),                \
}

static struct ieee80211_rate realtek_rates[] = {
	RATETAB_ENT(10, 0x1, 0),
	RATETAB_ENT(20, 0x2, 0),
	RATETAB_ENT(55, 0x4, 0),
	RATETAB_ENT(110, 0x8, 0),
	RATETAB_ENT(60, 0x10, 0),
	RATETAB_ENT(90, 0x20, 0),
	RATETAB_ENT(120, 0x40, 0),
	RATETAB_ENT(180, 0x80, 0),
	RATETAB_ENT(240, 0x100, 0),
	RATETAB_ENT(360, 0x200, 0),
	RATETAB_ENT(480, 0x400, 0),
	RATETAB_ENT(540, 0x800, 0),
};

static struct ieee80211_supported_band realtek_band_2ghz = {
	.n_channels = ARRAY_SIZE(realtek_2ghz_channels),
	.channels = realtek_2ghz_channels,
	.n_bitrates = realtek_g_rates_size,
	.bitrates = realtek_g_rates,
	.ht_cap.cap = realtek_g_htcap,
	.ht_cap.ht_supported = true,
};

static struct ieee80211_supported_band realtek_band_5ghz = {
        .n_channels = ARRAY_SIZE(realtek_5ghz_a_channels),
        .channels = realtek_5ghz_a_channels,
        .n_bitrates = realtek_a_rates_size,
        .bitrates = realtek_a_rates,
        .ht_cap.cap = realtek_a_htcap,
        .ht_cap.ht_supported = true,
};

enum RTK_CFG80211_EVENT {
	CFG80211_CONNECT_RESULT = 0,
	CFG80211_ROAMED = 1,
	CFG80211_DISCONNECTED = 2,
	CFG80211_IBSS_JOINED = 3,
	CFG80211_NEW_STA = 4,
	CFG80211_SCAN_DONE = 5,
	CFG80211_DEL_STA = 6,
	CFG80211_SS_RESULT = 7,
	CFG80211_REMAIN_ON_CHNL_EXPIRED = 8,
	CFG80211_RX_MGMT = 9,
        CFG80211_NULL = 10,
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
    stringiz(CFG80211_REMAIN_ON_CHNL_EXPIRED),
    stringiz(CFG80211_RX_MGMT),
};

//==================================================

//init cfg80211 driver
//=========================================================
#if 1

struct cfg80211_ops vwlan_cfg80211_ops = {
	.add_virtual_intf = realtek_cfg80211_add_iface_vface,
	.del_virtual_intf = realtek_cfg80211_del_iface_vface,
	.change_virtual_intf = realtek_cfg80211_change_iface_vface,
	.add_key = realtek_cfg80211_add_key_vface,
	.del_key = realtek_cfg80211_del_key_vface,
	.get_key = realtek_cfg80211_get_key_vface,
	.set_default_key = realtek_cfg80211_set_default_key_vface,
	.set_default_mgmt_key = realtek_cfg80211_set_default_mgmt_key_vface,
	#if 0
	.add_beacon = realtek_cfg80211_add_beacon,
	.set_beacon = realtek_cfg80211_set_beacon,
	.del_beacon = realtek_cfg80211_del_beacon,
	#endif
	.add_station = realtek_cfg80211_add_station_vface,
	.del_station = realtek_cfg80211_del_station_vface,
	.change_station = realtek_cfg80211_change_station_vface,
	.get_station = realtek_cfg80211_get_station_vface,
	.dump_station = realtek_cfg80211_dump_station_vface,
    #if 0//def CONFIG_MAC80211_MESH
	.add_mpath = realtek_cfg80211_add_mpath,
	.del_mpath = realtek_cfg80211_del_mpath,
	.change_mpath = realtek_cfg80211_change_mpath,
	.get_mpath = realtek_cfg80211_get_mpath,
	.dump_mpath = realtek_cfg80211_dump_mpath,
	.set_mesh_params = realtek_cfg80211_set_mesh_params,
	.get_mesh_params = realtek_cfg80211_get_mesh_params,
    #endif
	.change_bss = realtek_cfg80211_change_bss_vface,
	//.set_txq_params = realtek_cfg80211_set_txq_params,
	//.set_channel = realtek_cfg80211_set_channel,
	.suspend = realtek_cfg80211_suspend_vface,
	.resume = realtek_cfg80211_resume_vface,
	.scan = realtek_cfg80211_scan_vface,
	.assoc = realtek_cfg80211_assoc_vface,
    #if 0
	.auth = realtek_cfg80211_auth,
	.assoc = realtek_cfg80211_assoc,
	.deauth = realtek_cfg80211_deauth,
	.disassoc = realtek_cfg80211_disassoc,
    #endif
	.join_ibss = realtek_cfg80211_join_ibss_vface,
	.leave_ibss = realtek_cfg80211_leave_ibss_vface,
	.set_wiphy_params = realtek_cfg80211_set_wiphy_params_vface,
	.set_tx_power = realtek_cfg80211_set_tx_power_vface,
	.get_tx_power = realtek_cfg80211_get_tx_power_vface,
	.set_power_mgmt = realtek_cfg80211_set_power_mgmt_vface,
	.set_wds_peer = realtek_cfg80211_set_wds_peer_vface,
	.rfkill_poll = realtek_cfg80211_rfkill_poll_vface,
	//CFG80211_TESTMODE_CMD(ieee80211_testmode_cmd)
	.set_bitrate_mask = realtek_cfg80211_set_bitrate_mask_vface,
	.connect = realtek_cfg80211_connect_vface,
	.disconnect = realtek_cfg80211_disconnect_vface,
    #ifdef CONFIG_P2P_RTK_SUPPORT
	.remain_on_channel = realtek_remain_on_channel_vface,
	.cancel_remain_on_channel = realtek_cancel_remain_on_channel_vface,
    #endif
//	.channel_switch = realtek_cfg80211_channel_switch_vface,		//barry
//	.mgmt_tx = realtek_mgmt_tx_vface,								//barry
	.mgmt_frame_register = realtek_mgmt_frame_register_vface,
	//.dump_survey = realtek_dump_survey,//survey_dump
	.start_ap = realtek_start_ap_vface,
	.change_beacon = realtek_change_beacon_vface,
	.stop_ap = realtek_stop_ap_vface,
    #if 0
	.sched_scan_start = realtek_cfg80211_sscan_start,
	.sched_scan_stop = realtek_cfg80211_sscan_stop,
	.set_bitrate_mask = realtek_cfg80211_set_bitrate,
	.set_cqm_txe_config = realtek_cfg80211_set_txe_config,
    #endif
};


int vwlan_realtek_interface_add(struct vwlan_priv *priv,
					  struct vwlan_rtknl *rtk, const char *name,
					  enum nl80211_iftype type)
{
 	struct net_device *ndev;

	printk("%s:%d in\n",__FUNCTION__,__LINE__);

	printk("name = %s, type = %d\n", name, type);

	ndev = priv->dev;
	if (!ndev)
	{
		printk("ndev = NULL !!\n");
		free_netdev(ndev);
		return -1;
	}
#if 1//move to vwlan init
	strcpy(ndev->name, name);
#endif
	dev_net_set(ndev, wiphy_net(rtk->wiphy));

	priv->wdev.wiphy = rtk->wiphy;

	ndev->ieee80211_ptr = &priv->wdev;

	SET_NETDEV_DEV(ndev, wiphy_dev(rtk->wiphy));

	priv->wdev.netdev = ndev;
	priv->wdev.iftype = type;

	SET_NETDEV_DEVTYPE(ndev, &vwlan_wiphy_type);

	printk("%s:%d out\n",__FUNCTION__,__LINE__);

	register_netdev(ndev);

	rtk->ndev_add = ndev;

	printk("add priv=[%p] wdev=[%p] ndev=[%p]\n", priv, &priv->wdev, ndev);
	//rtk_add_priv(priv, rtk);

	printk("%s:%d out\n",__FUNCTION__,__LINE__);
	return 0;

}


int vwlan_realtek_cfg80211_init(struct vwlan_rtknl *rtk, struct vwlan_priv *priv, int idx)
{
	struct wiphy *wiphy = rtk->wiphy;
	bool band_2gig = false, band_5gig = false;
	int ret;

	printk("%s:%d in\n",__FUNCTION__,__LINE__);

	//printk("rtk=%p wiphy=%p\n", rtk, wiphy);

	wiphy->mgmt_stypes = rtw_cfg80211_default_mgmt_stypes; /*cy wang*/

	wiphy->signal_type=CFG80211_SIGNAL_TYPE_UNSPEC;

    wiphy->max_scan_ssids = SSID_SCAN_AMOUNT;

    wiphy->max_num_pmkids = MAX_NUM_PMKIDS;

    wiphy->max_scan_ie_len = SCAN_IE_LEN_MAX;
    wiphy->max_num_pmkids = MAX_NUM_PMKIDS;
	wiphy->max_remain_on_channel_duration = MAX_REMAIN_ON_CHANNEL_DURATION; /*cy wang p2p related*/

	wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM ;
	wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME ;
	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL ;/*cy wang ; p2p must use it*/

	wiphy->interface_modes = BIT(NL80211_IFTYPE_AP)|
							BIT(NL80211_IFTYPE_STATION) | //_eric_cfg station mandatory ??
							BIT(NL80211_IFTYPE_ADHOC) |
							BIT(NL80211_IFTYPE_P2P_CLIENT)|
							BIT(NL80211_IFTYPE_P2P_GO)|
							BIT(NL80211_IFTYPE_P2P_DEVICE); //wrt-adhoc

	set_wiphy_dev(wiphy, rtk->dev); //return wiphy->dev.parent;

	memcpy(wiphy->perm_addr, rtk->root_mac, ETH_ALEN);
	//memcpy(priv->pmib->dot11Bss.bssid, wiphy->perm_addr, ETH_ALEN);

	if(idx == 0)
		band_5gig = true;
	else
		band_2gig = true;

	if(band_2gig)
	{
		printk("set 2G\n");
		realtek_band_2ghz.ht_cap.mcs.rx_mask[0] = 0xff;
		realtek_band_2ghz.ht_cap.mcs.rx_mask[1] = 0xff;
		wiphy->bands[IEEE80211_BAND_2GHZ] = &realtek_band_2ghz;
		wiphy->bands[IEEE80211_BAND_5GHZ] = NULL;
		wiphy->bands[IEEE80211_BAND_60GHZ] = NULL;
	}

	if(band_5gig)
	{
		printk("set 5G\n");
		realtek_band_5ghz.ht_cap.mcs.rx_mask[0] = 0xff;
		realtek_band_5ghz.ht_cap.mcs.rx_mask[1] = 0xff;
		wiphy->bands[IEEE80211_BAND_2GHZ] = NULL;
		wiphy->bands[IEEE80211_BAND_5GHZ] = &realtek_band_5ghz;
		wiphy->bands[IEEE80211_BAND_60GHZ] = NULL;
	}

	wiphy->cipher_suites = cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

	ret = wiphy_register(wiphy);

	if (ret < 0) {
		printk("couldn't register wiphy device\n");
		return ret;
	}

	rtk->wiphy_registered = true;

	printk("%s:%d out\n",__FUNCTION__,__LINE__);

	return 0;
}

static void  vwlan_rtk_create_dev(struct vwlan_rtknl *rtk,int idx)
{
	printk("%s:%d in\n",__FUNCTION__,__LINE__);
	/* define class here */
	unsigned char zero[] = {0, 0, 0, 0, 0, 0};
    rtk->cl = class_create(THIS_MODULE, vwlan_rtk_dev_name[idx]);

    /* create first device */
    rtk->dev = device_create(rtk->cl, NULL, vwlan_rtk_wifi_dev[idx], NULL, vwlan_rtk_dev_name[idx]);

 	dev_set_name(rtk->dev, vwlan_rtk_dev_name[idx]);
  	printk("Device Name = %s \n", dev_name(rtk->dev));

	//init rtk phy root name
	sprintf(rtk->root_ifname, "wlan%d", idx);

	printk("root_ifname = %s \n", rtk->root_ifname);

	memcpy(rtk->root_mac, vwlan_rtk_fake_addr[idx], ETH_ALEN);

	printk("root_mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
		rtk->root_mac[0],rtk->root_mac[1],rtk->root_mac[2],rtk->root_mac[3],rtk->root_mac[4],rtk->root_mac[5]);


	printk("%s:%d out\n",__FUNCTION__,__LINE__);
}

struct vwlan_rtknl *vwlan_realtek_cfg80211_create(void)
{
	struct wiphy *wiphy = NULL;
	struct vwlan_rtknl *rtk = NULL;

	printk("%s:%d\n",__FUNCTION__,__LINE__);

	/* create a new wiphy for use with cfg80211 */
	wiphy = wiphy_new(&vwlan_cfg80211_ops, sizeof(struct vwlan_rtknl));
	if (!wiphy) {
		printk("couldn't allocate wiphy device\n");
		return NULL;
	}
	else
		printk("new a wiphy=%p\n", wiphy);

	rtk = wiphy_priv(wiphy);
	rtk->wiphy = wiphy;
	//rtk->priv = priv;	 //mark_dual2

	printk("new a rtk=%p\n", rtk);

	//sync to global rtk_phy
	if(vwlan_rtk_phy_idx > VWLAN_RTK_MAX_WIFI_PHY)
	{
		printk("ERROR!! rtk_phy_idx >  RTK_MAX_WIFI_PHY\n");
		wiphy_free(wiphy);
		return NULL;
	}
	vwlan_rtk_create_dev(rtk, vwlan_rtk_phy_idx);
	vwlan_rtk_phy[vwlan_rtk_phy_idx] = rtk;

	printk("rtk_phy_idx=%d\n",vwlan_rtk_phy_idx);

	vwlan_rtk_phy_idx++;

	//priv->rtk = rtk ; //mark_dual2
	printk("%s:%d\n",__FUNCTION__,__LINE__);

	return rtk;
}

extern vwlan_dev_config_t vwlan_dev_table[];
int vwlan_cfg80211_init(void)
{

	struct vwlan_rtknl *rtk = NULL;
	struct net_device *dev = NULL, *vwlan = NULL;
	struct vwlan_priv *priv = NULL;
	int idx = 0;

	printk("%s:%d\n",__FUNCTION__,__LINE__);

	for(idx=0;idx<2;idx++)
	{
		//rtk_nl, wiphy create
		rtk = vwlan_realtek_cfg80211_create();

		//device create
		dev = alloc_etherdev(sizeof(struct vwlan_priv));
		if (!dev)
			printk("create new net_dev fail !!!!\n");

		strcpy(dev->name, "wlan%d");

		vwlan = dev_get_by_name(&init_net, "vwlan");
		dev->netdev_ops = vwlan->netdev_ops;
		//dev->netdev_ops = &vwlan_netdev_ops;
		priv = (struct vwlan_priv *)netdev_priv(dev);

		priv->dev = dev;
		priv->rtk = rtk;
		rtk->priv = priv;

		printk("\n\n### wiphy=%p rtk=%p idx=%d dev=%p priv=%p\n\n",
			rtk->wiphy, rtk, idx, dev, priv);

		vwlan_realtek_cfg80211_init(rtk, priv, idx);

		vwlan_realtek_interface_add(priv, rtk, rtk->root_ifname, NL80211_IFTYPE_STATION);

		memcpy(dev->dev_addr, vwlan_dev_table[idx+1].dev_addr,  ETH_ALEN);
		vwlan_dev_table[idx+1].dev = dev;
		dev_num +=1;

		printk("idx=%d create dev->name = %s\n", idx, dev->name);
	}
	printk("%s:%d\n",__FUNCTION__,__LINE__);

	return 0;
}

#if 0
#define NLVENTER	printk("\n[rtk_nl80211]%s +++\n", (char *)__FUNCTION__)
#define NLVEXIT printk("[rtk_nl80211]%s ---\n\n", (char *)__FUNCTION__)
#define NLVINFO printk("[rtk_nl80211]%s %d\n", (char *)__FUNCTION__, __LINE__)
#define NLVNOT printk("[rtk_nl80211]%s !!! NOT implement YET !!!\n", (char *)__FUNCTION__)
#else
#define NLVENTER	do {} while(0)
#define NLVEXIT		do {} while(0)
#define NLVINFO		do {} while(0)
#define NLVNOT		do {} while(0)
#endif

#if 0
#define VFACE_DEBUG(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#else
#define VFACE_DEBUG(fmt, args...) do {} while(0)
#endif


struct cfg80211_scan_request *scan_req_pointer = NULL;

void vwlan_get_pointer_from_name(char *dev_name,
							struct wiphy **wiphy,
							struct net_device **dev,
							struct wireless_dev **wdev)
{
	//find wiphy
		if(!strncmp(dev_name, "wlan0", 5))
		{
			*wiphy = vwlan_rtk_phy[0]->wiphy;
			*dev = vwlan_rtk_phy[0]->priv->dev;
			*wdev = &(vwlan_rtk_phy[0]->priv->wdev);
		}
		else if(!strncmp(dev_name, "wlan1", 5))
		{
			*wiphy = vwlan_rtk_phy[1]->wiphy;
			*dev = vwlan_rtk_phy[1]->priv->dev;
			*wdev = &(vwlan_rtk_phy[1]->priv->wdev);
		}
		else
			printk("###[%s] Wrong dev name: %s !!!\n", __FUNCTION__, dev_name);

		return;

}

void vwlan_cfg80211_indicate_event(vwlan_cfg80211_event_ind_t *cfg)
{
	char tempbuf[33];

	struct wiphy *wiphy = NULL;
	struct net_device *dev = NULL;
	struct wireless_dev *wdev = NULL;
    int freq;
    struct ieee80211_channel channel_t;
    struct ieee80211_channel *channel_ptr;
	struct cfg80211_bss *bss = NULL;

    channel_ptr = &channel_t;
	vwlan_get_pointer_from_name(cfg->dev_name, &wiphy, &dev, &wdev);

    if(cfg->cmd!=CFG80211_SS_RESULT && cfg->cmd!=CFG80211_REMAIN_ON_CHNL_EXPIRED)// reduce too much message
	LDEBUG("[from left 2nd]:cmd[%d][%s]\n", cfg->cmd, cfg80211_event_str[cfg->cmd] );


	switch(cfg->cmd)
	{
		case CFG80211_CONNECT_RESULT:

#if 0
			printk("[get_bss]ch[%d] bssid[%02x%02x%02x:%02x%02x%02x] ssid=%s ssidlen=%d\n",cfg->connect_result.chnl,
			cfg->connect_result.bssid1[0],cfg->connect_result.bssid1[1],cfg->connect_result.bssid1[2],cfg->connect_result.bssid1[3],
			cfg->connect_result.bssid1[4],cfg->connect_result.bssid1[5],(char *)cfg->connect_result.ssid,cfg->connect_result.ssidlen);
#endif
            if(cfg->connect_result.channel_ch < 14)
                freq = ieee80211_channel_to_frequency(cfg->connect_result.channel_ch, IEEE80211_BAND_2GHZ);
            else
                freq = ieee80211_channel_to_frequency(cfg->connect_result.channel_ch, IEEE80211_BAND_5GHZ);


            channel_ptr = ieee80211_get_channel(wiphy, freq);

			bss = cfg80211_get_bss(wiphy,channel_ptr, cfg->connect_result.bssid1,
					cfg->connect_result.ssid, cfg->connect_result.ssidlen,WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);

            //&cfg->connect_result.channel,

			if(bss==NULL)
			{
				bss = cfg80211_inform_bss(wiphy,
					channel_ptr,
					cfg->connect_result.mac,
					cfg->connect_result.timestamp,
					cfg->connect_result.capability,
					cfg->connect_result.beacon_prd,
					cfg->connect_result.ie,
					cfg->connect_result.ie_len,
					cfg->connect_result.rssi, GFP_ATOMIC);
				if(bss)
					cfg80211_put_bss(wiphy, bss);
				else
					printk("bss = null\n");
			}

#if 0
			printk("[connect_result] assoc_req_len=%d assoc_rsp_len=%d\n",
			cfg->connect_result.assoc_req_len,cfg->connect_result.assoc_rsp_len);
#endif

			cfg80211_connect_result(dev, cfg->connect_result.bssid2,
					cfg->connect_result.assoc_req, cfg->connect_result.assoc_req_len,
					cfg->connect_result.assoc_rsp, cfg->connect_result.assoc_rsp_len,
					WLAN_STATUS_SUCCESS, GFP_KERNEL);

			LDEBUG("finish CFG80211_CONNECT_RESULT\n");
			break;
		case CFG80211_ROAMED:
			//do nothing
			break;
		case CFG80211_DISCONNECTED:
			cfg80211_disconnected(dev, 0, NULL, 0, GFP_KERNEL);
			break;
		case CFG80211_IBSS_JOINED:
			//cfg80211_ibss_joined(dev, cfg->ibss_joined.bssid, NULL, GFP_KERNEL);
			cfg80211_ibss_joined(dev, cfg->ibss_joined.bssid, GFP_KERNEL);		//barry
			break;
		case CFG80211_NEW_STA:
			cfg80211_new_sta(dev, cfg->new_sta.mac, &cfg->new_sta.sinfo, GFP_KERNEL);
			netif_wake_queue(dev); //wrt-vap
			break;
		case CFG80211_SCAN_DONE:
			if (scan_req_pointer) {
				cfg80211_scan_done(scan_req_pointer, false);
				scan_req_pointer = NULL;
			}
			break;
		case CFG80211_DEL_STA:
			cfg80211_del_sta(dev, cfg->del_sta.mac, GFP_KERNEL);
			break;
		case CFG80211_SS_RESULT:
        {

            if(cfg->connect_result.channel_ch < 14)
                freq = ieee80211_channel_to_frequency(cfg->connect_result.channel_ch, IEEE80211_BAND_2GHZ);
            else
                freq = ieee80211_channel_to_frequency(cfg->connect_result.channel_ch, IEEE80211_BAND_5GHZ);


            channel_ptr = ieee80211_get_channel(wiphy, freq);

			bss = cfg80211_inform_bss(wiphy,
					//&cfg->connect_result.channel, //!!wrong ways don't use!!
					channel_ptr,
					cfg->connect_result.mac,
					cfg->connect_result.timestamp,
					cfg->connect_result.capability,
					cfg->connect_result.beacon_prd,
					cfg->connect_result.ie,
					cfg->connect_result.ie_len,
					cfg->connect_result.rssi, GFP_ATOMIC);
			if(bss){
				cfg80211_put_bss(wiphy, bss);
			}else{
    			LDEBUG("bss = null\n");
            }
            #if 1 // just for debug
        	memset(tempbuf, '\0', 33);
			memcpy(tempbuf, cfg->connect_result.ie+2, cfg->connect_result.ie[1]);

			LDEBUG("SSID=[%s][%02x%02x%02x:%02x%02x%02x]\n",
                tempbuf,
				cfg->connect_result.mac[0],cfg->connect_result.mac[1],cfg->connect_result.mac[2],
				cfg->connect_result.mac[3],cfg->connect_result.mac[4],cfg->connect_result.mac[5]);
            #endif
			#if 0
			LDEBUG("band=[%d]\n",channel_ptr->band);
			LDEBUG("center_freq=[%d]\n",channel_ptr->center_freq);
			LDEBUG("hw_value=[%d]\n",channel_ptr->hw_value);
			LDEBUG("flags=[%d]\n",channel_ptr->flags);
			LDEBUG("beacon_found=[%d]\n",channel_ptr->beacon_found);
			LDEBUG("orig_flags=[%d]\n",channel_ptr->orig_flags);
            #endif
            #if 0
			LDEBUG(" timestamp=0x%08x cap=0x%02x bcn_prd=%d ie_len=%d rssi=%d\n", cfg->connect_result.timestamp,
				cfg->connect_result.capability, cfg->connect_result.beacon_prd, cfg->connect_result.ie_len,cfg->connect_result.rssi);
			#endif

			break;
        }
		case CFG80211_REMAIN_ON_CHNL_EXPIRED://P2P_ind_event
		    NDEBUG2("CFG80211_REMAIN_ON_CHNL_EXPIRED\n");
			cfg80211_remain_on_channel_expired(wdev,
				cfg->remain_on_chnl_expired.remain_on_ch_cookie,
				&cfg->remain_on_chnl_expired.remain_on_ch_channel,
				GFP_KERNEL);
			break;
		case CFG80211_RX_MGMT://P2P_ind_event
		    LDEBUG("CFG80211_RX_MGMT\n");
			/*									//barry
			cfg80211_rx_mgmt(wdev,
				cfg->rx_mgmt.freq,
				cfg->rx_mgmt.sig_dbm,
				cfg->rx_mgmt.rx_buf,
				cfg->rx_mgmt.rx_len,
				cfg->rx_mgmt.flags,
				GFP_ATOMIC);
			*/
			break;
		default:
			LDEBUG("Unknown Event !!\n");
			break;
	}
}


void vwlan_cfg80211_cmd(vwlan_cfg80211_t *cfg)
{
	printk("%s: cmd=%d dev_name=%s\n",__FUNCTION__, cfg->cmd, cfg->dev_name);

	_vwlan_rpc_cfg(cfg);

	return cfg->ret_val;		//barry ??? since this is void type
}

static struct wireless_dev *realtek_cfg80211_add_iface_vface(struct wiphy *wiphy,
						      const char *name,
						      enum nl80211_iftype type,
						      u32 *flags,
						      struct vif_params *params)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_ADD_IFACE;
	if(name != NULL)
		memcpy(cfg.dev_name, name, strlen(name));
	cfg.add_iface.type = type;

	VFACE_DEBUG("[add_iface] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[add_iface] type=%d\n", cfg.add_iface.type);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);

	NLVEXIT;

	return &rtk->priv->wdev;
}

static int realtek_cfg80211_del_iface_vface(struct wiphy *wiphy,
				     struct wireless_dev *wdev)
{
	vwlan_cfg80211_t cfg;

   	NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_DEL_IFACE;
	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, strlen(wdev->netdev->name));

	VFACE_DEBUG("[del_iface] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_change_iface_vface(struct wiphy *wiphy,
					struct net_device *ndev,
					enum nl80211_iftype type, u32 *flags,
					struct vif_params *params)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_CHANGE_IFACE;
	if(ndev->name != NULL)
		memcpy(cfg.dev_name, ndev->name, strlen(ndev->name));
	cfg.change_iface.type = type;

	VFACE_DEBUG("[change_iface] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_add_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr,
				   struct key_params *params)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_ADD_KEY;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.add_key.key_index = key_index;
	cfg.add_key.pairwise = pairwise;
	if(mac_addr != NULL)
		memcpy(cfg.add_key.mac_addr, mac_addr, ETH_ALEN);
	cfg.add_key.cipher = params->cipher;
	cfg.add_key.key_len = params->key_len;
	if(params->key != NULL)
		memcpy(cfg.add_key.key, params->key, params->key_len);

	VFACE_DEBUG("[add_key] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[add_key] key_index=%d\n", cfg.add_key.key_index);
	VFACE_DEBUG("[add_key] pairwise=%d\n", cfg.add_key.pairwise);
	VFACE_DEBUG("[add_key] mac_addr[5]=0x%x\n", cfg.add_key.mac_addr[5]);
	VFACE_DEBUG("[add_key] cipher=0x%x\n", cfg.add_key.cipher);
	VFACE_DEBUG("[add_key] key_len=%d\n", cfg.add_key.key_len);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_del_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_DEL_KEY;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.del_key.key_index = key_index;
	cfg.del_key.pairwise = pairwise;
	if(mac_addr != NULL)
		memcpy(cfg.del_key.mac_addr, mac_addr, ETH_ALEN);

	VFACE_DEBUG("[del_key] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[del_key] key_index=%d\n", cfg.del_key.key_index);
	VFACE_DEBUG("[del_key] pairwise=%d\n", cfg.del_key.pairwise);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_get_key_vface(struct wiphy *wiphy, struct net_device *dev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr, void *cookie,
				   void (*callback) (void *cookie,
						     struct key_params *))
{
	vwlan_cfg80211_t cfg;
	struct key_params params;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));
	memset(&params, 0, sizeof(struct key_params));

	cfg.cmd = CMD_CFG80211_GET_KEY;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.get_key.key_index = key_index;
	cfg.get_key.pairwise = pairwise;
	if(mac_addr != NULL)
		memcpy(cfg.get_key.mac_addr, mac_addr, ETH_ALEN);

	VFACE_DEBUG("[get_key] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[get_key] key_index=%d\n", cfg.get_key.key_index);
	VFACE_DEBUG("[get_key] pairwise=%d\n", cfg.get_key.pairwise);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	//Do Callback
	params.key_len = cfg.get_key.key_len;
	params.cipher = cfg.get_key.cipher;
	if(cfg.get_key.key != NULL)
		memcpy(params.key, cfg.get_key.key, cfg.get_key.key_len);

	VFACE_DEBUG("[get_key] key_len=%d\n", cfg.get_key.key_len);
	VFACE_DEBUG("[get_key] cipher=0x%x\n", cfg.get_key.cipher);

	callback(cookie, &params);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_default_key_vface(struct wiphy *wiphy,
					   struct net_device *dev,
					   u8 key_index, bool unicast,
					   bool multicast)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_DEFAULT_KEY;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.set_default_key.key_index = key_index;
	cfg.set_default_key.unicast = unicast;
	cfg.set_default_key.multicast = multicast;

	VFACE_DEBUG("[set_default_key] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_default_key] key_index=%d\n", cfg.set_default_key.key_index);
	VFACE_DEBUG("[set_default_key] unicast=%d\n", cfg.set_default_key.unicast);
	VFACE_DEBUG("[set_default_key] multicast=%d\n", cfg.set_default_key.multicast);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_default_mgmt_key_vface(struct wiphy *wiphy,
					     struct net_device *dev,
					     u8 key_idx)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_DEFAULT_MGMT_KEY;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.set_default_mgmt_key.key_idx = key_idx;

	VFACE_DEBUG("[set_default_mgmt_key] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_default_mgmt_key] key_index=%d\n", cfg.set_default_mgmt_key.key_idx);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_add_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac, struct station_parameters *params)
{
	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_ADD_STATION;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(mac)
		memcpy(cfg.add_station.mac, mac, ETH_ALEN);

	VFACE_DEBUG("[add_station] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[add_station] mac[5]=0x%x\n", cfg.add_station.mac[5]);

	NLVENTER;

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_del_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_DEL_STATION;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(mac)
		memcpy(cfg.del_station.mac, mac, ETH_ALEN);

	VFACE_DEBUG("[del_station] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[del_station] mac[5]=0x%x\n", cfg.del_station.mac[5]);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_change_station_vface(struct wiphy *wiphy,
				    struct net_device *dev,
				    u8 *mac,
				    struct station_parameters *params)
{

    if(is_zero_mac(mac)){
        NDEBUG3("zero mac, return!\n");
        return 0;
    }

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));
	cfg.cmd = CMD_CFG80211_CHANGE_STATION;

	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(mac)
		memcpy(cfg.change_station.mac, mac, ETH_ALEN);
	cfg.change_station.sta_flags_set = params->sta_flags_set;

	VFACE_DEBUG("[change_station] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[change_station] mac[5]=%02x:%02x:%02x:%02x:%02x:%02x\n",
		cfg.change_station.mac[0],cfg.change_station.mac[1],cfg.change_station.mac[2],
		cfg.change_station.mac[3],cfg.change_station.mac[4],cfg.change_station.mac[5]);
	VFACE_DEBUG("[change_station] sta_flags_set=%d\n", cfg.change_station.sta_flags_set);

#if 1//do this step in virtual side
	int err = cfg80211_check_station_change(wiphy, params,
						CFG80211_STA_AP_MLME_CLIENT);
	if (err)
	{
		printk("cfg80211_check_station_change error !! \n");
		return err;
	}
#endif

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_get_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac, struct station_info *sinfo)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_GET_STATION;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(mac)
		memcpy(cfg.get_station.mac, mac, ETH_ALEN);

	VFACE_DEBUG("[get_station] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[get_station] mac[5]=0x%x\n", cfg.get_station.mac[5]);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	memcpy(sinfo, &cfg.get_station.sinfo, sizeof(struct station_info));

	VFACE_DEBUG("[get_station] filled=%d\n", cfg.get_station.sinfo.filled);
	VFACE_DEBUG("[get_station] connected_time=%d\n", cfg.get_station.sinfo.connected_time);
	VFACE_DEBUG("[get_station] inactive_time=%d\n", cfg.get_station.sinfo.inactive_time);
	VFACE_DEBUG("[get_station] rx_bytes=%d\n", cfg.get_station.sinfo.rx_bytes);
	VFACE_DEBUG("[get_station] signal=%d\n", cfg.get_station.sinfo.signal);
	VFACE_DEBUG("[get_station] rx_packets=%d\n", cfg.get_station.sinfo.rx_packets);
	VFACE_DEBUG("[get_station] tx_packets=%d\n", cfg.get_station.sinfo.tx_packets);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_dump_station_vface(struct wiphy *wiphy, struct net_device *dev,
				 int idx, u8 *mac, struct station_info *sinfo)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_DUMP_STATION;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.dump_station.idx = idx;

	VFACE_DEBUG("[dump_station] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[dump_station] idx=%d\n", cfg.dump_station.idx);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	memcpy(sinfo, &cfg.dump_station.sinfo, sizeof(struct station_info));
	if(cfg.dump_station.mac)
		memcpy(mac, cfg.dump_station.mac,  ETH_ALEN);

	VFACE_DEBUG("[dump_station] mac[5]=0x%x\n", mac[5]);
	VFACE_DEBUG("[dump_station] filled=%d\n", cfg.dump_station.sinfo.filled);
	VFACE_DEBUG("[dump_station] connected_time=%d\n", cfg.dump_station.sinfo.connected_time);
	VFACE_DEBUG("[dump_station] inactive_time=%d\n", cfg.dump_station.sinfo.inactive_time);
	VFACE_DEBUG("[dump_station] rx_bytes=%d\n", cfg.dump_station.sinfo.rx_bytes);
	VFACE_DEBUG("[dump_station] signal=%d\n", cfg.dump_station.sinfo.signal);
	VFACE_DEBUG("[dump_station] rx_packets=%d\n", cfg.dump_station.sinfo.rx_packets);
	VFACE_DEBUG("[dump_station] tx_packets=%d\n", cfg.dump_station.sinfo.tx_packets);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_change_bss_vface(struct wiphy *wiphy,
				struct net_device *dev,
				struct bss_parameters *params)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_CHANGE_BSS;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.change_bss.use_short_preamble = params->use_short_preamble;
	cfg.change_bss.basic_rates_len = params->basic_rates_len;
	if(params->basic_rates != NULL)
		memcpy(cfg.change_bss.basic_rates, params->basic_rates, params->basic_rates_len);

	VFACE_DEBUG("[change_bss] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[change_bss] use_short_preamble=%d\n", cfg.change_bss.use_short_preamble);
	VFACE_DEBUG("[change_bss] basic_rates_len=%s\n", cfg.change_bss.basic_rates_len);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_suspend_vface(struct wiphy *wiphy, struct cfg80211_wowlan *wow)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SUSPEND;
	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);
	if(rtk->root_ifname != NULL)
		memcpy(cfg.dev_name, rtk->root_ifname, VIF_NAME_SIZE);

	VFACE_DEBUG("[suspend] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_resume_vface(struct wiphy *wiphy)
{
   	NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_RESUME;
	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);
	if(rtk->root_ifname != NULL)
		memcpy(cfg.dev_name, rtk->root_ifname, VIF_NAME_SIZE);

	VFACE_DEBUG("[resume] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}


    // left side exe here ; right side exe realtek_cfg80211_scan()   LLSCAN
static int realtek_cfg80211_scan_vface(struct wiphy *wiphy,
			  struct cfg80211_scan_request *request)
{


	int idx=0;
	vwlan_cfg80211_t cfg;

        if(request==NULL)
                return;

    //LDEBUG("\n");

	memset(&cfg, 0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SCAN;

	if(request->wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, request->wdev->netdev->name, strlen(request->wdev->netdev->name));

	if(request->ssids) /*we just take care first ssid now*/
		{
		LDEBUG("ssid[%s],len[%d]\n", request->ssids->ssid,request->ssids->ssid_len);
        //error: 'struct cfg80211_scan' has no member named 'ssid_len'
		cfg.scan.ssid_len = request->ssids->ssid_len;
		memcpy(cfg.scan.ssid, request->ssids->ssid, request->ssids->ssid_len);
		}

	if(request->ie)
	{
    	LDEBUG("IEs_len[%d]\n", request->ie_len);
		cfg.scan.ie_len = request->ie_len;
		memcpy(cfg.scan.ie, request->ie, request->ie_len);
	}

	cfg.scan.n_channels = request->n_channels;
	if(request->n_channels > 20){
		printk("[%s:%d] There is no channel space !!!!!\n",__FUNCTION__,__LINE__);
	}else{
    	LDEBUG("scan ch list:\n");
		for(idx=0; idx<request->n_channels; idx++){
			//memcpy(&cfg.scan.channels[idx], request->channels[idx], sizeof(struct ieee80211_channel));
			cfg.scan.cfg_channels[idx]=ieee80211_frequency_to_channel(request->channels[idx]->center_freq);
           	printk("ch[%d]", cfg.scan.cfg_channels[idx]);
		}
    	printk("\n");
	}


	//must fix here !!!!!
	//cfg.scan.request = request;

	//for scan done notify: scan_req
	scan_req_pointer = request;
   	//LDEBUG("scan_req_pointer[%p]\n", request);

	VFACE_DEBUG("[scan] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[scan] ssid_len=%d\n", cfg.scan.ssid_len);
	VFACE_DEBUG("[scan] ie_len=%d\n", cfg.scan.ie_len);
	VFACE_DEBUG("[scan] n_channels=%d\n", cfg.scan.n_channels);

	NLVENTER;

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_join_ibss_vface(struct wiphy *wiphy, struct net_device *dev,
			       struct cfg80211_ibss_params *ibss_param)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_JOIN_IBSS;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.join_ibss.ssid_len = ibss_param->ssid_len;
	cfg.join_ibss.privacy = ibss_param->privacy;
	if(ibss_param->ssid != NULL)
		memcpy(cfg.join_ibss.ssid, ibss_param->ssid, ibss_param->ssid_len);


	VFACE_DEBUG("[join_ibss] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[join_ibss] ssid_len=%d\n", cfg.join_ibss.ssid_len);
	VFACE_DEBUG("[join_ibss] privacy=%d\n", cfg.join_ibss.privacy);
	VFACE_DEBUG("[join_ibss] dev_name=%s\n", cfg.join_ibss.ssid);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_leave_ibss_vface(struct wiphy *wiphy, struct net_device *dev)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_LEAVE_IBSS;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));

	VFACE_DEBUG("[leave_ibss] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_wiphy_params_vface(struct wiphy *wiphy, u32 changed)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_WIPHY_PARAMS;
	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);
	if(rtk->root_ifname != NULL)
		memcpy(cfg.dev_name, rtk->root_ifname, VIF_NAME_SIZE);
	cfg.set_wiphy_params.changed = changed;
	cfg.set_wiphy_params.retry_short = wiphy->retry_short;
	cfg.set_wiphy_params.retry_long = wiphy->retry_long;
	cfg.set_wiphy_params.rts_threshold = wiphy->rts_threshold;

	VFACE_DEBUG("[set_wiphy_params] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_wiphy_params] changed=0x%x\n", cfg.set_wiphy_params.changed);
	VFACE_DEBUG("[set_wiphy_params] retry_short=%d\n", cfg.set_wiphy_params.retry_short);
	VFACE_DEBUG("[set_wiphy_params] retry_long=%d\n", cfg.set_wiphy_params.retry_long);
	VFACE_DEBUG("[set_wiphy_params] rts_threshold=%d\n", cfg.set_wiphy_params.rts_threshold);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_tx_power_vface(struct wiphy *wiphy,
				  enum nl80211_tx_power_setting type, int mbm)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_TX_POWER;
	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);
	if(rtk->root_ifname != NULL)
		memcpy(cfg.dev_name, rtk->root_ifname, VIF_NAME_SIZE);
	cfg.set_tx_power.type = type;
	cfg.set_tx_power.mbm = mbm;

	VFACE_DEBUG("[set_tx_power] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_tx_power] type=%d\n", cfg.set_tx_power.type);
	VFACE_DEBUG("[set_tx_power] mbm=%d\n", cfg.set_tx_power.mbm);

	//Do RPC Send Cmd
	//vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_get_tx_power_vface(struct wiphy *wiphy,
				  struct wireless_dev *wdev, int *dbm)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_GET_TX_POWER;
	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, strlen(wdev->netdev->name));

	VFACE_DEBUG("[get_tx_power] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	VFACE_DEBUG("[get_tx_power] dbm=%d\n", cfg.get_tx_power.dbm);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_power_mgmt_vface(struct wiphy *wiphy, struct net_device *dev,
				    bool enabled, int timeout)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_POWER_MGMT;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.set_power_mgmt.enabled = enabled;
	cfg.set_power_mgmt.timeout = timeout;

	VFACE_DEBUG("[set_power_mgmt] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_power_mgmt] enabled=%d\n", cfg.set_power_mgmt.enabled);
	VFACE_DEBUG("[set_power_mgmt] timeout=%d\n", cfg.set_power_mgmt.timeout);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_set_wds_peer_vface(struct wiphy *wiphy, struct net_device *dev,
				  u8 *addr)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_WDS_PEER;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(addr)
		memcpy(cfg.set_wds_peer.addr, addr, ETH_ALEN);

	VFACE_DEBUG("[set_wds_peer] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_wds_peer] addr[5]=0x%x\n", cfg.set_wds_peer.addr[5]);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static void realtek_cfg80211_rfkill_poll_vface(struct wiphy *wiphy)
{

    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_RFKILL_POLL;
	struct vwlan_rtknl *rtk = wiphy_priv(wiphy);
	if(rtk->root_ifname != NULL)
		memcpy(cfg.dev_name, rtk->root_ifname, VIF_NAME_SIZE);

	VFACE_DEBUG("[refill_poll] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return;
}

static int realtek_cfg80211_set_bitrate_mask_vface(struct wiphy *wiphy,
				      struct net_device *dev,
				      const u8 *addr,
				      const struct cfg80211_bitrate_mask *mask)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_SET_BITRATE_MASK;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	if(addr)
		memcpy(cfg.set_bitrate_mask.addr, addr, ETH_ALEN);
	if(mask)
		memcpy(&cfg.set_bitrate_mask.mask, mask, ETH_ALEN);

	VFACE_DEBUG("[set_bitrate_mask] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[set_bitrate_mask] addr[5]=0x%x\n", cfg.set_bitrate_mask.addr[5]);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_connect_vface(struct wiphy *wiphy, struct net_device *dev,
					  struct cfg80211_connect_params *sme)
{
    int idx;
	vwlan_cfg80211_t cfg;
    //NDEBUG3("\n");
	memset(&cfg, 0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_CONNECT;

	if(dev->name){
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
        LDEBUG("name[%s]\n",cfg.dev_name);
   }

	if(sme->bssid){
		memcpy(cfg.connect.bssid, sme->bssid, ETH_ALEN);
        LDEBUG("bssid\n");
        printMac(cfg.connect.bssid);
    }else{
		cfg.connect.no_bssid = 1;
        LDEBUG("no bssid\n");
    }


	if(sme->ssid != NULL && sme->ssid_len){
		memcpy(cfg.connect.ssid, sme->ssid, sme->ssid_len);
    	cfg.connect.ssid_len = sme->ssid_len;
        cfg.connect.ssid[sme->ssid_len]='\0';
        LDEBUG("ssid[%s]\n",cfg.connect.ssid);
    }

    if(sme->ie && sme->ie_len){
	cfg.connect.ie_len = sme->ie_len;
		memcpy(cfg.connect.ie, sme->ie, sme->ie_len);
        LDEBUG("ie len[%d]\n",cfg.connect.ie_len);
    }


	memcpy(&cfg.connect.crypto, &sme->crypto, sizeof(struct cfg80211_crypto_settings));
    //LDEBUG("cipher_group[0x%X]\n",sme->crypto.cipher_group);
    LDEBUG("n_ciphers_pairwise[%d]\n",sme->crypto.n_ciphers_pairwise);
    LDEBUG("control_port_no_encrypt[%d]\n",sme->crypto.control_port_no_encrypt);


	cfg.connect.key_idx = sme->key_idx;

	if(sme->key && sme->key_len){
		memcpy(cfg.connect.key, sme->key, sme->key_len);
        cfg.connect.key_len = sme->key_len;
        LDEBUG("key_len[%d]\n",cfg.connect.key_len);
        LDEBUG("key[%s]\n",sme->key);

    }

	NLVENTER;

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_disconnect_vface(struct wiphy *wiphy,
						  struct net_device *dev, u16 reason_code)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_DISCONNECT;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));
	cfg.disconnect.reason_code = reason_code;

	VFACE_DEBUG("[disconnect] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[disconnect] reason_code=%d\n", cfg.disconnect.reason_code);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

#ifdef CONFIG_P2P_RTK_SUPPORT
int realtek_remain_on_channel_vface(struct wiphy *wiphy,
    struct wireless_dev *wdev,
	struct ieee80211_channel *channel,
	unsigned int duration,
	u64 *cookie)
{
	NLVENTER;

	vwlan_cfg80211_t cfg;

    // first , report remain on channel OK!
    cfg80211_ready_on_channel(wdev, *cookie, channel, duration, GFP_KERNEL);

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));
	cfg.cmd = CMD_CFG80211_REMAIN_ON_CHANNEL;

	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, strlen(wdev->netdev->name));

	if(channel != NULL){
        memcpy(&cfg.remain_on_channel.remain_on_ch_channel, channel, sizeof(struct ieee80211_channel));
        cfg.remain_on_channel.re_channel = (u8)ieee80211_frequency_to_channel(channel->center_freq);
    }

	cfg.remain_on_channel.duration = duration;
	cfg.remain_on_channel.cookie = *cookie;

    LDEBUG("remain on channle for[%d]ms, cookie[%llx]\n",duration,(long long unsigned int)cfg.remain_on_channel.cookie);
	vwlan_cfg80211_cmd(&cfg);
    #if 0
	VFACE_DEBUG("[remain_on_channel] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[remain_on_channel] chnl_req=%d\n", cfg.remain_on_channel.channel.center_freq);
	VFACE_DEBUG("[remain_on_channel] duration=%d\n", cfg.remain_on_channel.duration);
	VFACE_DEBUG("[remain_on_channel] *cookie=%d\n", cfg.remain_on_channel.cookie);
#endif

	NLVEXIT;
	return cfg.ret_val;
}
static int realtek_cancel_remain_on_channel_vface(struct wiphy *wiphy,
			struct wireless_dev *wdev,	u64 cookie)
{
	NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));
	cfg.cmd = CMD_CFG80211_CANCEL_REMAIN_ON_CHANNEL;

	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, strlen(wdev->netdev->name));
	cfg.cancel_remain_on_channel.cookie = (long long unsigned int )cookie;

	VFACE_DEBUG("[cancel_remain_on_channel] dev_name=%s\n", cfg.dev_name);

	vwlan_cfg80211_cmd(&cfg);

	VFACE_DEBUG("[remain_on_channel] cookie=%d\n", cfg.cancel_remain_on_channel.cookie);

	NLVEXIT;

	return cfg.ret_val;
}
#endif //CONFIG_P2P
#if 0	//barry, because there is no cfg80211_csa_settings
static int realtek_cfg80211_channel_switch_vface(struct wiphy *wiphy,
			struct net_device *dev, struct cfg80211_csa_settings *params)

{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_CHANNEL_SWITCH;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));

	VFACE_DEBUG("[channel_switch] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}
#endif

#if 0	//barry, because there is no cfg80211_mgmt_tx_params struct
static int realtek_mgmt_tx_vface(struct wiphy *wiphy, struct wireless_dev *wdev,
           struct cfg80211_mgmt_tx_params *params,
           u64 *cookie)
{
	NLVENTER;
    static int mgmt_tx_id=0;
	u8 tx_ch = (u8)ieee80211_frequency_to_channel(params->chan->center_freq);
	vwlan_cfg80211_t cfg;
	memset(&cfg, 0, sizeof(vwlan_cfg80211_t));


    //=======cookie=======
	*cookie = 0;
    mgmt_tx_id++;
    if(mgmt_tx_id==0)
        mgmt_tx_id++;

    *cookie = mgmt_tx_id;
    //=======cookie=======

    //====report to cfg we've sent out thos packet
	cfg80211_mgmt_tx_status(wdev, *cookie, params->buf, params->len, true, GFP_KERNEL);//GFP_ATOMIC


    //====now prepare to send out thos packet

	cfg.cmd = CMD_CFG80211_MGMT_TX;

	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, strlen(wdev->netdev->name));

	//cfg.mgmt_tx.cookie = *cookie;
	//if(params->chan != NULL)
	//	memcpy(&cfg.mgmt_tx.chan, params->chan, sizeof(struct ieee80211_channel));

    cfg.mgmt_tx.mgmt_tx_channel= tx_ch;
	cfg.mgmt_tx.buf_len = params->len;

	if(params->len && params->buf){
		memcpy(cfg.mgmt_tx.buf, params->buf, params->len);
    }else{
        LDEBUG("mgmt packet len=0\n");
        return 0;
    }

	//VFACE_DEBUG("[mgmt_tx] dev_name=%s\n", cfg.dev_name);
	//VFACE_DEBUG("[mgmt_tx] *cookie=%s\n", cfg.mgmt_tx.cookie);
	//VFACE_DEBUG("[mgmt_tx] center_freq=%s\n", cfg.mgmt_tx.chan.center_freq);
	//VFACE_DEBUG("[mgmt_tx] len=%s\n", cfg.mgmt_tx.len);

	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;
    LDEBUG("exit\n");
	return cfg.ret_val;
}
#endif

static void realtek_mgmt_frame_register_vface(struct wiphy *wiphy,
				       struct  wireless_dev *wdev,
				       u16 frame_type, bool reg)
{
    #if 0
    NDEBUG3("\n");
	vwlan_cfg80211_t cfg;
	memset(&cfg, 0, sizeof(vwlan_cfg80211_t));
	cfg.cmd = CMD_CFG80211_MGMT_FRAME_REGISTER;
	if(wdev->netdev->name != NULL)
		memcpy(cfg.dev_name, wdev->netdev->name, IFNAMSIZ);
	NLVENTER;
	//Do RPC Send Cmd
	//vwlan_cfg80211_cmd(&cfg);
	NLVEXIT;
	//printk("[memt_frame_register] this function do not support now !!!\n");
	return cfg.ret_val;
    #endif
    NDEBUG3("\n");
    return 0;
}

static int realtek_start_ap_vface(struct wiphy *wiphy, struct net_device *dev,
			   struct cfg80211_ap_settings *info)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_START_AP;
	if(dev->name != NULL)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));

	//channel
	if(info->chandef.chan != NULL)
		memcpy(&cfg.start_ap.chan, &info->chandef.chan, sizeof(struct ieee80211_channel));
	cfg.start_ap.width = info->chandef.width;

	//set beacon
	//beacon_ie
	cfg.start_ap.beacon_ies_len = info->beacon.beacon_ies_len;
	if(info->beacon.beacon_ies_len < sizeof(cfg.start_ap.beacon_ies))
	{
		if(info->beacon.beacon_ies)
			memcpy(cfg.start_ap.beacon_ies, info->beacon.beacon_ies, info->beacon.beacon_ies_len);
	}
	else
		printk("[start_ap] LINE:%s is over size\n",__LINE__);

	//prob_ie
	cfg.start_ap.proberesp_ies_len = info->beacon.proberesp_ies_len;
	if(info->beacon.proberesp_ies_len < sizeof(cfg.start_ap.proberesp_ies))
	{
			if(info->beacon.proberesp_ies)
		memcpy(cfg.start_ap.proberesp_ies, info->beacon.proberesp_ies, info->beacon.proberesp_ies_len);
	}
	else
		printk("[start_ap] LINE:%s is over size\n",__LINE__);

	//assoc_ie
	cfg.start_ap.assocresp_ies_len = info->beacon.assocresp_ies_len;
	if(info->beacon.assocresp_ies_len < sizeof(cfg.start_ap.assocresp_ies))
	{
		if(info->beacon.assocresp_ies)
			memcpy(cfg.start_ap.assocresp_ies, info->beacon.assocresp_ies, info->beacon.assocresp_ies_len);
	}
	else
		printk("[start_ap] LINE:%s is over size\n",__LINE__);

	//ssid
	memcpy(cfg.start_ap.ssid, info->ssid, info->ssid_len);
	cfg.start_ap.ssid_len = info->ssid_len;
	cfg.start_ap.hidden_ssid = info->hidden_ssid;
	memcpy(&cfg.start_ap.crypto, &info->crypto, sizeof(struct cfg80211_crypto_settings));
	cfg.start_ap.auth_type = info->auth_type;


	VFACE_DEBUG("[start_ap] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[start_ap] width=%d\n", cfg.start_ap.width);
	VFACE_DEBUG("[start_ap] beacon_ies_len=%d\n", cfg.start_ap.beacon_ies_len);
	VFACE_DEBUG("[start_ap] proberesp_ies_len=%d\n", cfg.start_ap.proberesp_ies_len);
	VFACE_DEBUG("[start_ap] assocresp_ies_len=%d\n", cfg.start_ap.assocresp_ies_len);
	VFACE_DEBUG("[start_ap] ssid_len=%d\n", cfg.start_ap.ssid_len);
	VFACE_DEBUG("[start_ap] hidden_ssid=%d\n", cfg.start_ap.hidden_ssid);
	VFACE_DEBUG("[start_ap] auth_type=%d\n", cfg.start_ap.auth_type);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_change_beacon_vface(struct wiphy *wiphy, struct net_device *dev,
				struct cfg80211_beacon_data *beacon)
{
    NLVENTER;

	vwlan_cfg80211_t cfg;
	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_CHANGE_BEACON;
	if(dev->name)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));

	//beacon_ie
	cfg.change_beacon.beacon_ies_len = beacon->beacon_ies_len;
	if(beacon->beacon_ies_len < sizeof(cfg.change_beacon.beacon_ies))
	{
		if(beacon->beacon_ies)
			memcpy(cfg.change_beacon.beacon_ies, beacon->beacon_ies, beacon->beacon_ies_len);
	}
	else
		printk("[change_beacon] LINE:%d is over size\n",__LINE__);

	//prob_ie
	cfg.change_beacon.proberesp_ies_len = beacon->proberesp_ies_len;
	if(beacon->proberesp_ies_len < sizeof(cfg.change_beacon.proberesp_ies))
	{
		if(beacon->proberesp_ies)
			memcpy(cfg.change_beacon.proberesp_ies, beacon->proberesp_ies, beacon->proberesp_ies_len);
	}
	else
		printk("[change_beacon] LINE:%d is over size\n",__LINE__);

	//assoc_ie
	cfg.change_beacon.assocresp_ies_len = beacon->assocresp_ies_len;
	if(beacon->assocresp_ies_len < sizeof(cfg.change_beacon.assocresp_ies))
	{
		if(beacon->assocresp_ies)
			memcpy(cfg.change_beacon.assocresp_ies, beacon->assocresp_ies, beacon->assocresp_ies_len);
	}
	else
		printk("[change_beacon] LINE:%d is over size\n",__LINE__);

	VFACE_DEBUG("[change_beacon] dev_name=%s\n", cfg.dev_name);
	VFACE_DEBUG("[change_beacon] beacon_ies_len=%d\n", cfg.change_beacon.beacon_ies_len);
	VFACE_DEBUG("[change_beacon] proberesp_ies_len=%d\n", cfg.change_beacon.proberesp_ies_len);
	VFACE_DEBUG("[change_beacon] assocresp_ies_len=%d\n", cfg.change_beacon.assocresp_ies_len);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_stop_ap_vface(struct wiphy *wiphy, struct net_device *dev)
{
	vwlan_cfg80211_t cfg;

    NLVENTER;

	memset(&cfg, 0x0, sizeof(vwlan_cfg80211_t));

	cfg.cmd = CMD_CFG80211_STOP_AP;
	if(dev->name)
		memcpy(cfg.dev_name, dev->name, strlen(dev->name));

	VFACE_DEBUG("[stop_ap] dev_name=%s\n", cfg.dev_name);

	//Do RPC Send Cmd
	vwlan_cfg80211_cmd(&cfg);

	NLVEXIT;

	return cfg.ret_val;
}

static int realtek_cfg80211_assoc_vface(struct wiphy *wiphy, struct net_device *dev,
			   struct cfg80211_assoc_request *req)
{
    NDEBUG3("\n");
	return 0;
}


//=========================================================
#endif

//------------------------------------------------------------------------------

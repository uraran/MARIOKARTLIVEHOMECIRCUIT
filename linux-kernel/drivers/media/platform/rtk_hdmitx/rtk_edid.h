#ifndef _RTK_EDID_H_
#define _RTK_EDID_H_

#include "hdmitx.h"


int rtk_edid_header_is_valid(const u8 *raw_edid);
bool rtk_edid_block_valid(u8 *raw_edid, int block);
bool rtk_detect_hdmi_monitor(struct edid *edid);
struct edid *rtk_get_edid(asoc_hdmi_t *hdmi);
int rtk_do_probe_ddc_edid(unsigned char *buf, int len);
bool rtk_edid_is_valid(struct edid *edid);
int rtk_add_edid_modes(struct edid *edid, struct sink_capabilities_t *sink_cap);
void rtk_edid_to_eld(struct edid *edid, struct sink_capabilities_t *sink_cap);

void hdmi_print_edid(struct edid *edid);
void print_established_modes(u32 est_format);
void print_cea_modes(u64 cea_format);
void print_deep_color(u32 var);
void print_color_formats(u32 var);
void print_color_space(u8 var);
void hdmi_print_raw_edid(unsigned char *edid);

#endif /* _RTK_EDID_H_*/

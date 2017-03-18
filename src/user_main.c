/*
 * Copyright (c) 2017 Wojtek Kaniewski <wojtekka@toxygen.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "osapi.h"
#include "user_interface.h"

#define FRAME_CONTROL_TYPE_MANAGEMENT 0
#define FRAME_CONTROL_SUBTYPE_PROBE_REQUEST 4

struct frame_control
{
	uint8_t protocol_version:2;
	uint8_t type:2;
	uint8_t subtype:4;

	uint8_t to_ds:1;
	uint8_t from_ds:1;
	uint8_t more_frag:1;
	uint8_t retry:1;
	uint8_t pwr_mgt:1;
	uint8_t more_data:1;
	uint8_t protected_frame:1;
	uint8_t order:1;
};

struct probe_request
{
	struct frame_control frame_control;
	uint16_t duration;
	uint8_t destination[6];
	uint8_t source[6];
	uint8_t bssid[6];
	uint16_t number;
	uint8_t data[];
};

struct probe_tag
{
	uint8_t tag_number;
	uint8_t tag_length;
	uint8_t data[];
};

// Sniffer structure layout copied from https://espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf

struct RxControl 
{ 
	signed rssi:8;			// signal intensity of packet 
	unsigned rate:4; 
	unsigned is_group:1; 
	unsigned:1; 
	unsigned sig_mode:2;		// 0:is not 11n packet; non-0:is 11n packet; 
	unsigned legacy_length:12;	// if not 11n packet, shows length of packet. 
	unsigned damatch0:1; 
	unsigned damatch1:1; 
	unsigned bssidmatch0:1; 
	unsigned bssidmatch1:1; 
	unsigned MCS:7;			// if is 11n packet, shows the modulation 
					// and code used (range from 0 to 76)
	unsigned CWB:1; 		// if is 11n packet, shows if is HT40 packet or not 
	unsigned HT_length:16;		// if is 11n packet, shows length of packet. 
	unsigned Smoothing:1; 
	unsigned Not_Sounding:1; 
	unsigned:1; 
	unsigned Aggregation:1; 
	unsigned STBC:2; 
	unsigned FEC_CODING:1;		// if is 11n packet, shows if is LDPC packet or not. 
	unsigned SGI:1; 
	unsigned rxend_state:8; 
	unsigned ampdu_cnt:8; 
	unsigned channel:4;		//which channel this packet in. 
	unsigned:12; 
}; 

struct LenSeq
{
	u16 len; // length of packet 
	u16 seq; // serial number of packet, the high 12bits are serial number,
	         //    low 14 bits are Fragment number (usually be 0) 
	u8 addr3[6]; // the third address in packet 
}; 

struct sniffer_buf
{
	struct RxControl rx_ctrl; 
	u8 buf[36]; // head of ieee80211 packet 
	u16 cnt;     // number count of packet 
	struct LenSeq lenseq[1];  //length of packet 
};

struct sniffer_buf2
{ 
	struct RxControl rx_ctrl; 
	u8 buf[112]; //may be 240, please refer to the real source code
	u16 cnt;   
	u16 len;  //length of packet 
}; 

void ICACHE_FLASH_ATTR wifi_promiscuous_rx(uint8_t *buf, uint16 len)
{
	if (len == sizeof(struct sniffer_buf2))
	{
		/* Management packet */

		struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;

		len = sniffer->len;

		struct probe_request *probe_buf = (struct probe_request*) sniffer->buf;

		if (probe_buf->frame_control.type == FRAME_CONTROL_TYPE_MANAGEMENT && probe_buf->frame_control.subtype == FRAME_CONTROL_SUBTYPE_PROBE_REQUEST)
		{
			char ssid[33];
			struct probe_tag *tag = (struct probe_tag*) probe_buf->data;

			if (tag->tag_number == 0 && tag->tag_length > 0 && tag->tag_length <= 32)
			{
				memcpy(ssid, tag->data, tag->tag_length);
				ssid[tag->tag_length] = 0;
			}
			else
			{
				ssid[0] = 0;
			}
			os_printf("probe,%02x:%02x:%02x:%02x:%02x:%02x,%d,%s\r\n",
				probe_buf->source[0],
				probe_buf->source[1],
				probe_buf->source[2],
				probe_buf->source[3],
				probe_buf->source[4],
				probe_buf->source[5],
				sniffer->rx_ctrl.rssi,
				ssid);
		}
	}
	else if (len % 10 == 0)
	{
		/* AMPDU? */
	}
	else if (len == sizeof(struct RxControl))
	{
		/* Unreliable packet */
	}
}

static os_timer_t ping_timer;

void ICACHE_FLASH_ATTR ping_handler(void *arg)
{
	os_printf("ping,,,\r\n");
}

void ICACHE_FLASH_ATTR system_init_done(void)
{
	wifi_set_opmode(STATION_MODE);
	wifi_promiscuous_enable(1);
	wifi_set_promiscuous_rx_cb(wifi_promiscuous_rx);
}

void ICACHE_FLASH_ATTR user_init(void)
{
	uart_div_modify(0, UART_CLK_FREQ / 9600);
	system_init_done_cb(system_init_done);
	os_timer_setfn(&ping_timer, ping_handler, NULL);
	os_timer_arm(&ping_timer, 5000, true);
	os_printf("\r\ninit,,,\r\n");
}


/*
 * OpenConnect (SSL + DTLS) VPN client
 *
 * Copyright © 2008-2015 Intel Corporation.
 *
 * Author: David Woodhouse <dwmw2@infradead.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <config.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include "openconnect-internal.h"

void destroy_esp_ciphers(struct esp *esp)
{
	if (esp->cipher) {
		gnutls_cipher_deinit(esp->cipher);
		esp->cipher = NULL;
	}
	if (esp->hmac) {
		gnutls_hmac_deinit(esp->hmac, NULL);
		esp->hmac = NULL;
	}
}

static int init_esp_ciphers(struct openconnect_info *vpninfo, struct esp *esp,
			    gnutls_mac_algorithm_t macalg, gnutls_cipher_algorithm_t encalg)
{
	gnutls_datum_t enc_key;
	int err;

	/* ∄ gnutls_cipher_get_key_size() */
	if (encalg == GNUTLS_CIPHER_AES_128_CBC)
		enc_key.size = 16;
	else if (encalg == GNUTLS_CIPHER_AES_256_CBC)
		enc_key.size = 32;
	else
		return -EINVAL;
	
	enc_key.data = esp->secrets;

	err = gnutls_cipher_init(&esp->cipher, encalg, &enc_key, NULL);
	if (err) {
		vpn_progress(vpninfo, PRG_ERR,
			     _("Failed to initialise ESP cipher: %s\n"),
			     gnutls_strerror(err));
		return -EIO;
	}

	err = gnutls_hmac_init(&esp->hmac, macalg,
			       esp->secrets + enc_key.size,
			       gnutls_hmac_get_len(macalg));
	if (err) {
		vpn_progress(vpninfo, PRG_ERR,
			     _("Failed to initialize ESP HMAC: %s\n"),
			     gnutls_strerror(err));
		destroy_esp_ciphers(esp);
	}
	return 0;
}

int setup_esp_keys(struct openconnect_info *vpninfo)
{
	gnutls_mac_algorithm_t macalg;
	gnutls_cipher_algorithm_t encalg;
	int ret;

	if (vpninfo->dtls_state == DTLS_DISABLED)
		return -EOPNOTSUPP;

	switch (vpninfo->esp_enc) {
	case 0x02:
		encalg = GNUTLS_CIPHER_AES_128_CBC;
		break;
	case 0x05:
		encalg = GNUTLS_CIPHER_AES_256_CBC;
		break;
	default:
		return -EINVAL;
	}

	switch (vpninfo->esp_hmac) {
	case 0x01:
		macalg = GNUTLS_MAC_MD5;
		break;
	case 0x02:
		macalg = GNUTLS_MAC_SHA1;
		break;
	default:
		return -EINVAL;
	}

	ret = gnutls_rnd(GNUTLS_RND_NONCE, &vpninfo->esp_in.spi,
			 sizeof(vpninfo->esp_in.secrets) + sizeof(vpninfo->esp_in.spi));
	if (ret) {
		vpn_progress(vpninfo, PRG_ERR,
			     _("Failed to generate random keys for ESP: %s\n"),
			     gnutls_strerror(ret));
		return -EIO;
	}

	ret = init_esp_ciphers(vpninfo, &vpninfo->esp_out, macalg, encalg);
	if (ret)
		return ret;

	ret = init_esp_ciphers(vpninfo, &vpninfo->esp_in, macalg, encalg);
	if (ret) {
		destroy_esp_ciphers(&vpninfo->esp_out);
		return ret;
	}

	vpninfo->dtls_state = DTLS_SECRET;
	return 0;
}


int decrypt_and_queue_esp_packet(struct openconnect_info *vpninfo, unsigned char *esp, int len)
{
	struct pkt *pkt;
	unsigned char hmac_buf[20];
	const int ivsize = 16; /* We don't support anything different... yet. */

	if (len < 20 + ivsize)
		return -EINVAL;

	if (memcmp(esp, vpninfo->esp_in.spi, 4)) {
		vpn_progress(vpninfo, PRG_DEBUG,
			     _("Received ESP packet with invalid SPI %02x%02x%02x%02x\n"),
			     esp[0], esp[1], esp[2], esp[3]);
		return -EINVAL;
	}

	/* XXX: Implement seq checking. Record the highest seq# received, and keep
	   a small bitmap covering a sliding window just before that, so out-of-order
	   packets can be accepted within reason but each packet only once. */

	gnutls_hmac(vpninfo->esp_in.hmac, esp, len - 12);
	gnutls_hmac_output(vpninfo->esp_in.hmac, hmac_buf);
	if (memcmp(hmac_buf, esp + len - 12, 12)) {
		vpn_progress(vpninfo, PRG_DEBUG,
			     _("Received ESP packet with invalid HMAC\n"));
		return -EINVAL;
	}

	gnutls_cipher_set_iv(vpninfo->esp_in.cipher, esp + 8, ivsize);

	len -= 20 + ivsize;

	pkt = malloc(sizeof(struct pkt) + len);
	if (!pkt)
		return -ENOMEM;

	if (gnutls_cipher_decrypt2(vpninfo->esp_in.cipher,
				   pkt->data + 8 + ivsize, len,
				   pkt->data, len)) {
		printf("decrypt fail\n");
		return -EINVAL;
	}

	if (pkt->data[len - 1] != 0x04 && pkt->data[len - 1] != 0x29) {
		/* 0x05 is LZO compressed. */
		vpn_progress(vpninfo, PRG_ERR,
			     _("Received ESP packet with unrecognised payload type %02x\n"),
			     pkt->data[len-1]);
		return -EINVAL;
	}
	if (len <= 2 + pkt->data[len - 2]) {
		printf("Invalid padding length %02x in ESP\n",
		       pkt->data[len - 2]);
		return -EINVAL;
	}
	pkt->len = len - 2 + pkt->data[len - 2];

	queue_packet(&vpninfo->incoming_queue, pkt);
	return 0;
}
